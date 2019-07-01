//**************************************************************************/
// Copyright (c) 1998-2007 Autodesk, Inc.
// All rights reserved.
// 
// These coded instructions, statements, and computer programs contain
// unpublished proprietary information written by Autodesk, Inc., and are
// protected by Federal copyright law. They may not be disclosed to third
// parties or copied or duplicated in any form, in whole or in part, without
// the prior written consent of Autodesk, Inc.
//**************************************************************************/
// DESCRIPTION: Appwizard generated plugin
// AUTHOR: 
//***************************************************************************/

#include "maxProject3.h"
#include "../common/SimpleFile.h"
#include "../common/MyErrorProc.h"
#include "../common/Define.h"
#include <IGame/IGame.h>
#include <IGame/IConversionManager.h>
#include <IGame/IGameModifier.h>
#include <conio.h>
#include <iostream>
#include <hash_map>
#include <map>
#include <hash_set>
#include <algorithm>
#include <atlconv.h> 
#include <windows.h>
using namespace std;


/************************************************************************/
/* 

MD5Version <int:version>
commandline <string:commandline>

numJoints <int:numJoints>
numMeshes <int:numMeshes>
numObjects <int:numObjects>
numMaterials <int:numMaterials>

joints {
<string:name> <int:parentIndex> ( <vec3:position> ) ( <vec3:orientation> )
...
}

objects {
<string:name> <int:startMesh> <int:meshCount>
...
}

materials {
<string:name> <string:diffuseTexture> <string:opacityTexture> <int:twoSided> <float:transparency>
...
}

mesh {
meshindex <int:meshIndex>

shader <string:materialName> 

numverts <int:numVerts>
vert <int:vertexIndex> ( <vec2:texCoords> ) <int:startWeight> <int:weightCount>
...

numtris <int:numTriangles>
tri <int:triangleIndex> <int:vertIndex0> <int:vertIndex1> <int:vertIndex2>
...

numweights <int:numWeights>
weight <int:weightIndex> <int:jointIndex> <float:weightBias> ( <vec3:weightPosition> )
...

}
...

*/
/************************************************************************/

#define maxProject3_CLASS_ID	Class_ID(0x6857114d, 0x58a38332)


struct BoneInfo 
{
	BoneInfo()
		:
	SelfIndex(-1),
		ParentIndex(-1)
	{

	}
	MCHAR* Name;
	int SelfIndex;
	int ParentIndex;
	Point3 Pos;
	Quat Rot;

	bool operator < (const BoneInfo &b) const
	{
		//父节点小的在前面同时保证名字按顺序
		return MaxAlphaNumComp(Name,b.Name)<0;
	}
};

struct ObjectInfo 
{
	ObjectInfo()
		:StartMesh(0),
		MeshCount(0)
	{

	}

	MCHAR* Name;
	int StartMesh;
	int MeshCount;

};

struct MaterialInfo
{
	MaterialInfo()
		:Diffuse(NULL),
		Opacity(NULL),
		TwoSided(FALSE),
		Transparency(1.0f)
	{

	}
	MCHAR* Name;
	MCHAR* Diffuse;
	MCHAR* Opacity;
	BOOL TwoSided;
	float Transparency;
};

struct WeightInfo;

struct VertexInfo
{
	VertexInfo()
		:VertIndex(-1),
		TexCoordIndex(-1),
		WeightIndex(-1),
		WeightCount(-1)
	{

	}
	int VertIndex;
	int TexCoordIndex;
	Point3 Pos;
	Point2 UV;
	int WeightIndex;
	int WeightCount;

	vector<WeightInfo> Weights;
};

struct TriInfo
{
	int Index[3];
};

struct WeightInfo 
{
	WeightInfo()
		:Value(0.f)
	{

	}
	float Value;
	IGameNode* Bone;
	Point3 Offset;
};

//去除字符串右边空格
void Str_RTrim(WCHAR* pStr)
{
	WCHAR* pTmp = pStr + wcslen(pStr) - 1;
	while(*pTmp == L' ')
	{
		*pTmp = L'\0';
		pTmp--;
	}
}
//去除字符串左边空格
void StrLTrim(WCHAR* pStr)
{
	WCHAR* pTmp = pStr;
	while(*pTmp == L' ')
	{
		pTmp++;
	}
	while(*pTmp !=L'\0')
	{
		*pStr = *pTmp;
		pStr++;
		pTmp++;
	}
	*pStr=L'\0';
}
void Str_trim(WCHAR* pStr)
{
	WCHAR* pTmp = pStr;
	while(*pStr != L'\0')
	{
		if(*pStr != L' ')
		{
			*pTmp++ = *pStr;
		}
		++pStr;
	}
	*pTmp = L'\0';
}
//把字符串中的空格用下划线代替
void Str_replace_place_with_underline(WCHAR* pStr)
{
	StrLTrim(pStr);
	Str_RTrim(pStr);
	WCHAR* pTmp = pStr;
	while(*pStr != L'\0')
	{
		if(*pStr != L' ')
		{
			*pTmp++ = *pStr;
		}
		else
		{
			*pTmp++ = L'_';
		}
		++pStr;
	}
	*pTmp = L'\0';
}
//骨头排序按层级 以广度优先 按名字顺序
void BoneSort(vector<BoneInfo>& boneList)
{
	vector<BoneInfo> tmpList(boneList.begin(),boneList.end());
	boneList.clear();
	vector<BoneInfo>::iterator it=tmpList.begin();

	boneList.push_back(*it);
	tmpList.erase(it);

	int parentIndex=0;
	while (!tmpList.empty())
	{
		BoneInfo& parent=boneList.at(parentIndex);
		vector<BoneInfo>::iterator childIt=tmpList.begin();
		vector<BoneInfo> children;
		while (childIt!=tmpList.end())
		{
			if (childIt->ParentIndex==parent.SelfIndex)
			{
				children.push_back(*childIt);
				tmpList.erase(childIt);
				childIt=tmpList.begin();
			}
			else
				++childIt;
		}
		sort(children.begin(),children.end());
		boneList.insert(boneList.end(),children.begin(),children.end());
		++parentIndex;
	}
}

class maxProject3 : public SceneExport {
public:
	//Constructor/Destructor
	maxProject3();
	~maxProject3();

	int				ExtCount();					// Number of extensions supported
	const TCHAR *	Ext(int n);					// Extension #n (i.e. "3DS")
	const TCHAR *	LongDesc();					// Long ASCII description (i.e. "Autodesk 3D Studio File")
	const TCHAR *	ShortDesc();				// Short ASCII description (i.e. "3D Studio")
	const TCHAR *	AuthorName();				// ASCII Author name
	const TCHAR *	CopyrightMessage();			// ASCII Copyright message
	const TCHAR *	OtherMessage1();			// Other message #1
	const TCHAR *	OtherMessage2();			// Other message #2
	unsigned int	Version();					// Version number * 100 (i.e. v3.01 = 301)
	void			ShowAbout(HWND hWnd);		// Show DLL's "About..." box

	BOOL SupportsOptions(int ext, DWORD options);
	int  DoExport(const TCHAR *name,ExpInterface *ei,Interface *i, BOOL suppressPrompts=FALSE, DWORD options=0);


public:
	TimeValue _TvToDump;
	BOOL _CopyImages;
	BOOL _Compress;

	BOOL _LimitBoneNumPerMesh;
	int _MaxBonePerMesh;
	BOOL _DoomVersion;
	BOOL _HelperObject;

	bool _TargetExport;

	FILE* _OutFile;

	IGameScene * pIgame;

	int _BoneCount;
	int _MeshCount;
	int _ObjCount;
	int _MtlCount;

	vector<BoneInfo> _BoneList;
	hash_map<MCHAR*,int> _BoneIndexMap;
	vector<ObjectInfo> _ObjInfoList;
	vector<MaterialInfo> _MtlInfoList;

	int		SaveMd5Mesh(ExpInterface *ei, Interface *gi);

	void	DumpCount();

	void	CountNodes( IGameNode * pGameNode,BOOL isBoneChild=FALSE ) ;

	void	DumpBones();

	void DumpJoint( IGameNode * pGameNode,int& CurIndex,int ParentIndex=-1) 
	{
		int index=CurIndex;
		IGameObject * obj = pGameNode->GetIGameObject();
		if ((obj->GetIGameType()==IGameObject::IGAME_BONE&&
			(_HelperObject||obj->GetMaxObject()->SuperClassID()!=HELPER_CLASS_ID))||
			(_HelperObject&&obj->GetIGameType()==IGameObject::IGAME_HELPER))
		{
			GMatrix mat=pGameNode->GetWorldTM(_TvToDump);

			mat=ToRightHand(mat);

			Point3 pos=mat.Translation();
			Quat quat=mat.Rotation();

			if (quat.w<0)
			{
				quat=-quat;
			}

			BoneInfo bone;
			bone.Name=(WCHAR*)pGameNode->GetName();
			bone.SelfIndex=(int)_BoneList.size();
			bone.ParentIndex=ParentIndex;
			bone.Pos=pos;
			bone.Rot=quat;

			_BoneList.push_back(bone);

			++CurIndex;
		}

		pGameNode->ReleaseIGameObject();

		for(int i=0;i<pGameNode->GetChildCount();i++)
		{
			IGameNode * child = pGameNode->GetNodeChild(i);

			DumpJoint(child,CurIndex,index);
		}
	}

	GMatrix ToRightHand(const GMatrix& mat) const
	{
		Point3 pos=mat.Translation();
		Quat quat=mat.Rotation();
		Matrix3 ret=mat.ExtractMatrix3();
		//检测是否是镜像
		bool isMirrored=DotProd( CrossProd( ret.GetRow(0).Normalize(), ret.GetRow(1).Normalize() ).Normalize(), ret.GetRow(2).Normalize() ) < 0;
		//如果是则对镜像进行旋转的修正
		//修正方式还有待调整
		if (isMirrored)
		{
			float tmp;
			tmp=quat.x;
			quat.x=-quat.y;
			quat.y=tmp;
			tmp=quat.z;
			quat.z=quat.w;
			quat.w=-tmp;
		}
		ret.SetRotate(quat);
		ret.SetTrans(pos);
		return GMatrix(ret);
	}

	void DumpObjects() 
	{
		fprintf(_OutFile,"objects {\r\n");
		USES_CONVERSION;
		for (int i=0;i<_ObjCount;++i)
		{
			fprintf(_OutFile,"\t\"%s\" %d %d\r\n",W2A(_ObjInfoList.at(i).Name),_ObjInfoList.at(i).StartMesh,_ObjInfoList.at(i).MeshCount);
		}
		_ObjInfoList.clear();
		fprintf(_OutFile,"}\r\n\r\n");
	}

	void DumpMaterials() 
	{
		fprintf(_OutFile,"materials {\r\n");
		for (int i=0;i<_MtlCount;++i)
		{
			USES_CONVERSION;
			fprintf(_OutFile,"\t\"%s\" \"%s\" \"%s\" %d %f\r\n",W2A(_MtlInfoList.at(i).Name),
				W2A(GetFileName(_MtlInfoList.at(i).Diffuse)),W2A(GetFileName(_MtlInfoList.at(i).Opacity)),
				_MtlInfoList.at(i).TwoSided,_MtlInfoList.at(i).Transparency);
		}
		_MtlInfoList.clear();
		fprintf(_OutFile,"}\r\n\r\n");
	}

	void DumpMeshes() 
	{
		_MeshCount=0;
		for(int loop = 0; loop <pIgame->GetTopLevelNodeCount();loop++)
		{
			IGameNode * pGameNode = pIgame->GetTopLevelNode(loop);
			DumpMesh(pGameNode);

		}
	}

	void DumpMesh( IGameNode * pGameNode ) 
	{
		IGameObject * obj = pGameNode->GetIGameObject();
		if (!pGameNode->IsNodeHidden()&&
			obj->GetIGameType()==IGameObject::IGAME_MESH)
		{
			IGameMesh * gM = (IGameMesh*)obj;
			if(gM->InitializeData())
			{
				bool hasSkin=false;
				int numMod = obj->GetNumModifiers();
				if(numMod > 0)
				{
					int i=0;
					for(i=0;i<numMod;i++)
					{
						IGameModifier * gMod = obj->GetIGameModifier(i);
						if (gMod->IsSkin())
						{
							hasSkin=true;
							DumpSubMesh(pGameNode,gM,(IGameSkin*)gMod);
							break;
						}
					}
				}
				if (!hasSkin)
				{
					DumpSubMesh(pGameNode,gM,NULL);
				}
			}
			else
			{
				DebugPrint(L"BadObject\n");
			}
		}

		pGameNode->ReleaseIGameObject();

		for(int i=0;i<pGameNode->GetChildCount();i++)
		{
			IGameNode * child = pGameNode->GetNodeChild(i);

			DumpMesh(child);
		}
	}

	void DumpSubMesh( IGameNode * pGameNode,IGameMesh * gM ,IGameSkin* gSkin) 
	{
		Tab<int> matIDs=gM->GetActiveMatIDs();
		int matCount=matIDs.Count();
		hash_map<MCHAR*,vector<FaceEx *> > sameMatMap;

		for (int m=0;m<matCount;++m)
		{
			Tab<FaceEx *> faces=gM->GetFacesFromMatID(matIDs[m]);
			IGameMaterial * mat=gM->GetMaterialFromFace(faces[0]);
			vector<FaceEx *>& faceList=sameMatMap[(WCHAR*)mat->GetMaterialName()];
			for (int f=0;f<faces.Count();++f)
			{
				faceList.push_back(faces[f]);
			}
		}

		for (hash_map<MCHAR*,vector<FaceEx *> >::iterator it=sameMatMap.begin();
			it!=sameMatMap.end();++it)
		{
			DumpFaces(gM,it->second,gSkin,pGameNode,it->first);
		}
	}

	void CountMesh( IGameObject * obj) 
	{
		_ObjInfoList.at(_ObjCount-1).StartMesh=_MeshCount;
		IGameMesh * gM = (IGameMesh*)obj;
		//下面这段代码的作用是先罗列出所有该mesh使用的material,然后把该mesh中所有的face根据使用的material分到不同的组中，放在一个hash_map sameMatMap中,然后分不同的组的facelist传到splitMesh中统计改组face使用的骨骼数量，如果超过Maxbone数量就要把改组face(mesh)分拆。
		//为什么要这样？因为md5mesh中的一个mesh必须使用相同的material(shader,diffuse图片),可以查看md5mesh文件中mesh组的字段。
		if(gM->InitializeData())
		{
			Tab<int> matIDs=gM->GetActiveMatIDs();
			int matCount=matIDs.Count();

			hash_map<MCHAR*,vector<FaceEx *> > sameMatMap;

			for (int m=0;m<matCount;++m)
			{
				Tab<FaceEx *> faces=gM->GetFacesFromMatID(matIDs[m]);
				IGameMaterial * mat=gM->GetMaterialFromFace(faces[0]);
				CoutMtl(mat);

				if (_LimitBoneNumPerMesh)
				{
					vector<FaceEx *>& faceList=sameMatMap[(WCHAR*)mat->GetMaterialName()];
					for (int f=0;f<faces.Count();++f)
					{
						faceList.push_back(faces[f]);
					}
				}
			}

			bool hasSkin=false;
			if (_LimitBoneNumPerMesh)
			{
				int numMod = obj->GetNumModifiers();
				if(numMod > 0)
				{
					for(int i=0;i<numMod;i++)
					{
						IGameModifier * gMod = obj->GetIGameModifier(i);
						if (gMod->IsSkin())
						{
							hasSkin=true;
							for (hash_map<MCHAR*,vector<FaceEx *> >::iterator it=sameMatMap.begin();
								it!=sameMatMap.end();++it)
							{
								SplitMesh(gM,it->second,(IGameSkin*)gMod);//((IGameSkin*)gMod)->GetInitialPose()gM//
							}
							break;;
						}
					}
				}
			}
			if (!_LimitBoneNumPerMesh||!hasSkin)
			{
				//delete by yrz
				//_MeshCount+=matCount;
			}

			_ObjInfoList.at(_ObjCount-1).MeshCount=_MeshCount-_ObjInfoList.at(_ObjCount-1).StartMesh;
		}
		else
		{
			DebugPrint(L"BadObject\n");
		}
	}

	void CoutMtl( IGameMaterial* pGameMtl );

	TSTR GetFileName( MCHAR* path ) 
	{
		if (path)
		{
			TSTR strPath,strFile;
			SplitPathFile( path,&strPath,&strFile );
			return strFile;
		}
		return _T("");
	}

	void DumpFaces( IGameMesh * gM,vector<FaceEx *>& faces, IGameSkin* gSkin,IGameNode* pGameNode,MCHAR* matName) 
	{
		vector<VertexInfo> vertList;
		vector<TriInfo> triList;
		hash_set<MCHAR*> sameBoneMap;
		int oldBoneNum=0;
		int faceCount=(int)faces.size();
		int weightIndex=0;
		int usedFaceNum=0;
		for (int f=0;f<faceCount;++f)
		{
			oldBoneNum=(int)sameBoneMap.size();

			TriInfo tri;
			for (int i=0;i<3;++i)
			{
				VertexInfo info;
				info.VertIndex=faces[f]->vert[i];
				info.TexCoordIndex=faces[f]->texCoord[i];

				int vertCount=(int)vertList.size();
				int v=0;
				for (;v<vertCount;++v)
				{
					VertexInfo& curinfo=vertList.at(v);
					if (info.VertIndex==curinfo.VertIndex)
					{
						if (info.TexCoordIndex==curinfo.TexCoordIndex)
						{
							tri.Index[i]=v;
							break;
						}
						else
						{
							VertexUV(info, gM);

							if (info.UV.Equals(curinfo.UV))
							{
								tri.Index[i]=v;
								break;
							}

							info.WeightIndex=curinfo.WeightIndex;
							info.WeightCount=curinfo.WeightCount;
						}
					}
					else
					{
						VertexPos(gM, info);

						if (info.Pos.Equals(curinfo.Pos))
						{
							if (info.TexCoordIndex==curinfo.TexCoordIndex)
							{
								tri.Index[i]=v;
								break;
							}
							else
							{
								VertexUV(info, gM);

								if (info.UV.Equals(curinfo.UV))
								{
									tri.Index[i]=v;
									break;
								}

								info.WeightIndex=curinfo.WeightIndex;
								info.WeightCount=curinfo.WeightCount;
							}
						}
					}
				}
				if (v==vertCount)
				{
					VertexUV(info, gM);
					VertexPos(gM, info);

					if (-1==info.WeightIndex&&-1==info.WeightCount)
					{
						info.WeightIndex=weightIndex;
						info.WeightCount=gSkin?gSkin->GetNumberOfBones(info.VertIndex):1;

						weightIndex+=info.WeightCount>VERT_MAX_BONES?VERT_MAX_BONES:info.WeightCount;

						VertexWeight(info, gSkin, gM);
					}

					vertList.push_back(info);

					tri.Index[i]=(int)vertList.size()-1;

					for (int i=0;i<(int)info.Weights.size();++i)
					{
						IGameNode* bone=info.Weights.at(i).Bone;
						sameBoneMap.insert((WCHAR*)(bone?bone->GetName():_BoneList.at(0).Name));
					//	OutputDebugStringW(bone->GetName());
					//	OutputDebugStringW(L"\r\n");
					//	if(!bone)
						{
							//MessageBox(NULL,L"bone is NULL",L"BONE IS NULL",MB_OK);
						}
					}
				}
			}
			triList.push_back(tri);

			if (_LimitBoneNumPerMesh)
			{
				usedFaceNum++;
				if (oldBoneNum<_MaxBonePerMesh&&
					(int)sameBoneMap.size()>=_MaxBonePerMesh)
				{
					DumpVertex(pGameNode,matName,vertList, usedFaceNum, triList, weightIndex, (int)sameBoneMap.size());
					//WCHAR tip[100];
					//OutputDebugStringW(L"DumpFaces\r\n\r\n\r\n\r\n\r\n");
					//wsprintfW(tip,L"%d",_MeshCount);
					//OutputDebugStringW(tip);
				//	hash_set<MCHAR*>::iterator beg = sameBoneMap.begin(),
				//		end = sameBoneMap.end(),ite;
					//for(ite=beg;ite!=end;ite++)
					{
						//wsprintfW(tip,L"%s\r\n",(*ite));
						//OutputDebugStringW(tip);
					}
					sameBoneMap.clear();
					vertList.clear();
					triList.clear();
					weightIndex=0;
					usedFaceNum=0;
				}
			}
		}
		if (_LimitBoneNumPerMesh)
			faceCount=usedFaceNum;
		if ((int)sameBoneMap.size()>0)
			DumpVertex(pGameNode,matName,vertList, faceCount, triList, weightIndex, (int)sameBoneMap.size());
	}

	void DumpVertex(IGameNode* pGameNode,MCHAR* matName, vector<VertexInfo> &vertList, int faceCount,
		vector<TriInfo> &triList, int weightIndex,int boneNum) 
	{
		fprintf(_OutFile,"mesh {\r\n");
		USES_CONVERSION;
		fprintf(_OutFile,"\t// meshes: %s\r\n",W2A(pGameNode->GetName()));
		//WCHAR tip[100];
		//wsprintfW(tip,L"%s,%s\r\n",pGameNode->GetName(),matName);
		//OutputDebugStringW(tip);
		if (!_DoomVersion)
		{
			fprintf(_OutFile,"\tmeshindex %d\r\n\r\n",_MeshCount++);
		}
		CHAR* shaderName = NULL;
		
		for (int i=0;i<_MtlCount;++i)
		{
			if(strcmp(W2A(_MtlInfoList.at(i).Name),W2A(matName))==0)
			{
				shaderName = W2A(GetFileName(_MtlInfoList.at(i).Diffuse));
				break;
			}
		}

		fprintf(_OutFile,"\tshader \"%s\"\r\n\r\n",shaderName);

		fprintf(_OutFile,"\tnumverts %d\r\n",vertList.size());

		int vertCount=(int)vertList.size();
		for (int v=0;v<vertCount;++v)
		{
			VertexInfo& curinfo=vertList.at(v);

			fprintf(_OutFile,"\tvert %d ( %f %f ) %d %d\r\n",v,
				curinfo.UV.x,curinfo.UV.y,
				curinfo.WeightIndex,curinfo.WeightCount>VERT_MAX_BONES?VERT_MAX_BONES:curinfo.WeightCount);
		}

		fprintf(_OutFile,"\r\n\tnumtris %d\r\n",faceCount);

		int triCount=(int)triList.size();
		for (int t=0;t<triCount;++t)
		{
			TriInfo& tri=triList.at(t);

			fprintf(_OutFile,"\ttri %d %d %d %d\r\n",t,
				tri.Index[0],
				tri.Index[2],
				tri.Index[1]);
		}

		fprintf(_OutFile,"\r\n\tnumweights %d\r\n",weightIndex);

		for (int v=0;v<vertCount;++v)
		{
			VertexInfo& curinfo=vertList.at(v);

			int weightCount=(int)curinfo.Weights.size();
			for (int u=0;u<weightCount;++u)
			{
				WeightInfo& weight=curinfo.Weights.at(u);

				fprintf(_OutFile,"\tweight %d %d %f ( %f %f %f )\r\n",curinfo.WeightIndex+u,
					_BoneIndexMap[(WCHAR*)(weight.Bone?weight.Bone->GetName():_BoneList.at(0).Name)],
					weight.Value,
					weight.Offset.x,
					weight.Offset.y,
					weight.Offset.z);
			}
		}
		fprintf(_OutFile,"\r\n\t//bones %d\r\n",boneNum);

		fprintf(_OutFile,"}\r\n\r\n");
	}


	void VertexPos( IGameMesh * gM, VertexInfo &info ) 
	{
		info.Pos=gM->GetVertex(info.VertIndex);
	}


	void VertexUV( VertexInfo &info, IGameMesh * gM ) 
	{
		info.UV=gM->GetTexVertex(info.TexCoordIndex);
		if (info.UV.x>1.f)
			info.UV.x=1.f;
		if (info.UV.y<0.f)
			info.UV.y=-info.UV.y;
		else
			info.UV.y=1.f-info.UV.y;
	}

	//这段代码的作用是整理一个vertex的权重表，一个vertex最多有VERT_MAX_BONES个权重，如果原来权重多于VERT_MAX_BONES，就把权重最大的四个留下，并重新计算着VERT_MAX_BONES个权重，使之加起来和为1.
	//一个vertex的权重信息包括这个骨骼对这个vertex的权重值和这个顶点距离这个骨骼的距离，一个vertexinfo中包含了vertexweight的Index和weightcount,用这两个信息来找到他的weightinfo
	void VertexWeight( VertexInfo &info, IGameSkin* gSkin, IGameMesh * gM ) 
	{
		int weightCount=info.WeightCount;
		WeightInfo weights[VERT_MAX_BONES]={WeightInfo()};

		for (int b=0;b<weightCount;++b)
		{
			float curWeight=gSkin?gSkin->GetWeight(info.VertIndex,b):1;

			if (b<VERT_MAX_BONES)
			{
				weights[b].Value=curWeight;
				weights[b].Bone=gSkin?gSkin->GetIGameBone(info.VertIndex,b):NULL;
			}
			else 
			{
				for (int u=0;u<VERT_MAX_BONES;++u)
				{
					if (weights[u].Value<curWeight)
					{
						weights[u].Value=curWeight;
						weights[u].Bone=gSkin?gSkin->GetIGameBone(info.VertIndex,b):NULL;
						break;
					}
				}
			}
		}

		float totalWeight=0;
		if (weightCount>VERT_MAX_BONES)
			weightCount=VERT_MAX_BONES;
		for (int u=0;u<weightCount;++u)
		{
			totalWeight+=weights[u].Value;
		}
		for (int u=0;u<weightCount;++u)
		{
			weights[u].Value/=totalWeight;

			GMatrix initMat;
			//这里用GetInitBoneTM需要对应((IGameSkin*)gMod)->GetInitialPose()网格
			//但是基础动作的计算是用的第0帧的GetWorldTM
			//所有这里也对应网格就直接用gm了
			//if (!gSkin->GetInitBoneTM(weights[u].Bone,initMat))
			//{
			if (weights[u].Bone)
				initMat=weights[u].Bone->GetWorldTM(_TvToDump);
			else
				initMat.SetIdentity();
			//}
			initMat=ToRightHand(initMat);
			initMat=initMat.Inverse();
			weights[u].Offset=info.Pos*initMat;
			info.Weights.push_back(weights[u]);
		}
	}

	void SplitMesh( IGameMesh * gM, vector<FaceEx *>& faces, IGameSkin* gSkin ) 
	{
		//该函数的功能逻辑是找出这组face中所有顶点使用到的骨骼，并把每个顶点的骨头数量限制到VERT_MAX_BONES，然后把挑选出的有效骨骼加入到boneMap这个hash_set中（不重样的功能）,这样，就能得到最后需要用到的骨骼数量，如果骨骼数量大于_MaxBonePerMesh，就增加_MeshCount，注意，这里
		//并没有真正的分割mesh,只是算出了分割后的meshcount.
		hash_set<IGameNode*> boneMap;
		int meshBone=0;
		int faceCount=(int)faces.size();
		for (int f=0;f<faceCount;++f)
		{
			meshBone=(int)boneMap.size();
			//下面这段for循环代码的作用是找出这个face 三角形三个顶点VERT_MAX_BONES个对他影响最大的骨骼，丢弃其他影响较小的骨骼，并把这VERT_MAX_BONES个骨头记录在boneMap中，他们的影响权重记录在weights这个数组中
			for (int i=0;i<3;++i)
			{
				int weightCount=gSkin->GetNumberOfBones(faces[f]->vert[i]);
				WeightInfo weights[VERT_MAX_BONES]={WeightInfo()};

				for (int b=0;b<weightCount;++b)
				{
					float curWeight=gSkin->GetWeight(faces[f]->vert[i],b);

					if (b<VERT_MAX_BONES)
					{
						weights[b].Value=curWeight;
						weights[b].Bone=gSkin->GetIGameBone(faces[f]->vert[i],b);
					}
					else 
					{
						for (int u=0;u<VERT_MAX_BONES;++u)
						{
							if (weights[u].Value<curWeight)
							{
								weights[u].Value=curWeight;
								weights[u].Bone=gSkin->GetIGameBone(faces[f]->vert[i],b);
								break;
							}
						}
					}
				}

				if (weightCount>VERT_MAX_BONES)
					weightCount=VERT_MAX_BONES;

				for (int u=0;u<weightCount;++u)
				{
					boneMap.insert(weights[u].Bone);
				}
			}

			if (meshBone<_MaxBonePerMesh&&
				(int)boneMap.size()>=_MaxBonePerMesh)
			{
				_MeshCount++;
				//WCHAR tip[100];
				//OutputDebugStringW(L"SplitMesh\r\n\r\n\r\n\r\n\r\n");
				//wsprintfW(tip,L"%d",_MeshCount);
				//OutputDebugStringW(tip);
			//	hash_set<IGameNode*>::iterator beg = boneMap.begin(),
			//		end = boneMap.end(),ite;
			//	for(ite=beg;ite!=end;ite++)
				{
					//wsprintfW(tip,L"%s\r\n",(*ite)->GetName());
				//	OutputDebugStringW(tip);
				}
				boneMap.clear();
			}
		}
		if ((int)boneMap.size()>0)
		{
			_MeshCount++;
		}
	}


};



class maxProject3ClassDesc : public ClassDesc2 
{
public:
	virtual int IsPublic() 							{ return TRUE; }
	virtual void* Create(BOOL /*loading = FALSE*/) 		{ return new maxProject3(); }
	virtual const TCHAR *	ClassName() 			{ return GetString(IDS_CLASS_NAME); }
	virtual SClass_ID SuperClassID() 				{ return SCENE_EXPORT_CLASS_ID; }
	virtual Class_ID ClassID() 						{ return maxProject3_CLASS_ID; }
	virtual const TCHAR* Category() 				{ return GetString(IDS_CATEGORY); }

	virtual const TCHAR* InternalName() 			{ return _T("maxProject3"); }	// returns fixed parsable name (scripter-visible name)
	virtual HINSTANCE HInstance() 					{ return hInstance; }					// returns owning module handle
	

};


ClassDesc2* GetmaxProject3Desc() { 
	static maxProject3ClassDesc maxProject3Desc;
	return &maxProject3Desc; 
}





INT_PTR CALLBACK maxProject3OptionsDlgProc(HWND hWnd,UINT message,WPARAM,LPARAM lParam) {
	static maxProject3* imp = nullptr;

	switch(message) {
		case WM_INITDIALOG:
			imp = (maxProject3 *)lParam;
			CenterWindow(hWnd,GetParent(hWnd));

			return TRUE;

		case WM_CLOSE:
			EndDialog(hWnd, 0);
			return 1;
	}
	return 0;
}


//--- maxProject3 -------------------------------------------------------
maxProject3::maxProject3():_CopyImages(FALSE),
	_Compress(FALSE),
	_LimitBoneNumPerMesh(FALSE),
	_MaxBonePerMesh(36),
	_DoomVersion(TRUE),
	_HelperObject(FALSE)
{
	
}

maxProject3::~maxProject3() 
{

}

int maxProject3::ExtCount()
{
	#pragma message(TODO("Returns the number of file name extensions supported by the plug-in."))
	return 1;
}

const TCHAR *maxProject3::Ext(int /*i*/)
{		
	#pragma message(TODO("Return the 'i-th' file name extension (i.e. \"3DS\")."))
	return _T("md5mesh");
}

const TCHAR *maxProject3::LongDesc()
{
	#pragma message(TODO("Return long ASCII description (i.e. \"Targa 2.0 Image File\")"))
	return _T("md5mesh");
}
	
const TCHAR *maxProject3::ShortDesc() 
{			
	#pragma message(TODO("Return short ASCII description (i.e. \"Targa\")"))
	return _T("md5mesh");
}

const TCHAR *maxProject3::AuthorName()
{			
	#pragma message(TODO("Return ASCII Author name"))
	return _T("yaorongzhen");
}

const TCHAR *maxProject3::CopyrightMessage() 
{	
	#pragma message(TODO("Return ASCII Copyright message"))
	return _T("");
}

const TCHAR *maxProject3::OtherMessage1() 
{		
	//TODO: Return Other message #1 if any
	return _T("");
}

const TCHAR *maxProject3::OtherMessage2() 
{		
	//TODO: Return other message #2 in any
	return _T("");
}

unsigned int maxProject3::Version()
{				
	#pragma message(TODO("Return Version number * 100 (i.e. v3.01 = 301)"))
	return 100;
}

void maxProject3::ShowAbout(HWND /*hWnd*/)
{			
	// Optional
}

BOOL maxProject3::SupportsOptions(int /*ext*/, DWORD /*options*/)
{
	#pragma message(TODO("Decide which options to support.  Simply return true for each option supported by each Extension the exporter supports."))
	return TRUE;
}


int	maxProject3::DoExport(const TCHAR* name, ExpInterface* ei, Interface* i, BOOL suppressPrompts, DWORD options)
{
	#pragma message(TODO("Implement the actual file Export here and"))

	int result = TRUE;
	if(!suppressPrompts)
		DialogBoxParam(hInstance, 
				MAKEINTRESOURCE(IDD_PANEL), 
				GetActiveWindow(), 
				maxProject3OptionsDlgProc, (LPARAM)this);
	if(result > 0)
	{
		MyErrorProc pErrorProc;
		SetErrorCallBack(&pErrorProc);

		pIgame = GetIGameInterface();
		IGameConversionManager * cm = GetConversionManager();
		cm->SetCoordSystem(IGameConversionManager::IGAME_USER);
		UserCoord coord={
			1,	//hand
			1,	//X
			4,	//Y
			2,	//Z
			1,	//U
			0,	//V
		};
		cm->SetUserCoordSystem(coord);
		_TargetExport=(options & SCENE_EXPORT_SELECTED) ? true : false;
		pIgame->InitialiseIGame();
		pIgame->SetStaticFrame(0);

		_TvToDump=0;

		SimpleFile outFile(name,L"wb");
		_OutFile=outFile.File();

		result=SaveMd5Mesh(ei,i);

		MessageBox( GetActiveWindow(), name, _T("md5mesh finish"), 0 );

		pIgame->ReleaseIGame();
		_OutFile=NULL;
	}
	else
	{
		result = TRUE;
	}
	#pragma message(TODO("return TRUE If the file is exported properly"))
	return result;
}


int maxProject3::SaveMd5Mesh( ExpInterface *ei, Interface *gi )
{
	if (!_DoomVersion)
		fprintf(_OutFile,"MD5Version 4843\r\ncommandline \"by HoneyCat md5meshExporter v%d\"\r\n\r\n",Version());
	else
		fprintf(_OutFile,"MD5Version 10\r\ncommandline \"by HoneyCat md5meshExporter v%d\"\r\n\r\n",Version());

	DumpCount();

	DumpBones();

	if (!_DoomVersion)
	{
		DumpObjects();

		DumpMaterials();
	}

	DumpMeshes();

	fflush(_OutFile);
	return TRUE;
}


void maxProject3::DumpCount()
{
	_BoneCount=0;
	_MeshCount=0;
	_ObjCount=0;
	_MtlCount=0;

	for(int loop = 0; loop <pIgame->GetTopLevelNodeCount();loop++)
	{
		IGameNode * pGameNode = pIgame->GetTopLevelNode(loop);
		CountNodes(pGameNode);

	}

	fprintf(_OutFile,"numJoints %d\r\n",_BoneCount+1);
	fprintf(_OutFile,"numMeshes %d\r\n",_MeshCount);
	//if (!_DoomVersion)
	{
		fprintf(_OutFile,"numObjects %d\r\n",_ObjCount);	
		fprintf(_OutFile,"numMaterials %d\r\n\r\n",_MtlCount);
	}
	
}

void maxProject3::CountNodes( IGameNode * pGameNode,BOOL isBoneChild )
{
	IGameObject * obj = pGameNode->GetIGameObject();
	/*WCHAR nodename[100];
	wsprintfW(nodename,L"%s",(WCHAR*)pGameNode->GetName());
	
	if(wcscmp(nodename,L"Object01") == 0)
	{
		OutputDebugStringW(nodename);
	}
	if(wcscmp(nodename,L"Object02") == 0)
	{
		OutputDebugStringW(nodename);
	}*/
	if ((obj->GetIGameType()==IGameObject::IGAME_BONE&&
		(_HelperObject||obj->GetMaxObject()->SuperClassID()!=HELPER_CLASS_ID))||
		(_HelperObject&&obj->GetIGameType()==IGameObject::IGAME_HELPER))
	{
		_BoneCount++;
		isBoneChild=TRUE;
	}
	else if (!pGameNode->IsNodeHidden()&&
		obj->GetIGameType()==IGameObject::IGAME_MESH
		/*&&!isBoneChild*/)
	{
		_ObjCount++;
		//add by yrz
		_MeshCount++;
		ObjectInfo info;
		info.Name=(WCHAR*)pGameNode->GetName();
		_ObjInfoList.push_back(info);

		CountMesh(obj);
		isBoneChild=FALSE;
	}
	//fprintf(_OutFile,"%s %d\n",pGameNode->GetName(),obj->GetIGameType());
	pGameNode->ReleaseIGameObject();

	for(int i=0;i<pGameNode->GetChildCount();i++)
	{
		IGameNode * child = pGameNode->GetNodeChild(i);

		CountNodes(child,isBoneChild);
	}
}

void maxProject3::DumpBones() 
{
	fprintf(_OutFile,"joints {\r\n");

	BoneInfo bone;
	bone.Name=L"origin";
	bone.SelfIndex=(int)_BoneList.size();
	bone.ParentIndex=-1;
	bone.Pos=Point3::Origin;
	bone.Rot=Quat();

	_BoneList.push_back(bone);

	int index=1;
	for(int loop = 0; loop <pIgame->GetTopLevelNodeCount();loop++)
	{
		IGameNode * pGameNode = pIgame->GetTopLevelNode(loop);

		DumpJoint(pGameNode,index,0);
	}

	//为了保证和动画里面的顺序一致先排序
	BoneSort(_BoneList);

	int boneSize=(int)_BoneList.size();
	for (int b=0;b<boneSize;++b)
	{
		BoneInfo& info=_BoneList.at(b);
		if (info.ParentIndex>-1)
		{
			for (int p=0;p<_BoneList.size();++p)
			{
				BoneInfo& parentInfo=_BoneList.at(p);
				if (parentInfo.SelfIndex==info.ParentIndex)
				{
					info.ParentIndex=p;
					break;
				}
			}
		}
	}

	for (int b=0;b<_BoneList.size();++b)
	{
		BoneInfo& info=_BoneList.at(b);
		_BoneIndexMap[info.Name]=b;
		WCHAR tmp[100];
		wcscpy(tmp,info.Name);
		Str_replace_place_with_underline(tmp);
		USES_CONVERSION;
		fprintf(_OutFile,"\t\"%s\" %d ( %f %f %f ) ( %f %f %f )\r\n",W2A(tmp),info.ParentIndex,
			info.Pos.x,info.Pos.y,info.Pos.z,
			info.Rot.x,info.Rot.y,info.Rot.z);
	}

	fprintf(_OutFile,"}\r\n\r\n");
}

void maxProject3::CoutMtl( IGameMaterial* pGameMtl )
{
	int m=0;
	TSTR mtlName=pGameMtl->GetMaterialName();
	for (;m<_MtlCount;++m)
	{
		TSTR curName=_MtlInfoList.at(m).Name;
		if (mtlName==curName)
		{
			break;
		}
	}

	if (m==_MtlCount)
	{
		MaterialInfo mtlInfo;
		mtlInfo.Name=(WCHAR*)pGameMtl->GetMaterialName();

		int texCount = pGameMtl->GetNumberOfTextureMaps();
		for(int i=0;i<texCount;i++)
		{
			IGameTextureMap * tex = pGameMtl->GetIGameTextureMap(i);
			switch (tex->GetStdMapSlot())
			{
			case ID_DI:
				mtlInfo.Diffuse=(WCHAR*)tex->GetBitmapFileName();
				break;
			case ID_OP:
				mtlInfo.Opacity=(WCHAR*)tex->GetBitmapFileName();
				break;
			}
		}

		Mtl* m=pGameMtl->GetMaxMaterial();
		//半透明度
		mtlInfo.Transparency=m->GetXParency();

		if (m->IsSubClassOf(Class_ID(DMTL_CLASS_ID,0)))
		{
			StdMat* std = (StdMat *)m;
			//是否双面
			mtlInfo.TwoSided=std->GetTwoSided();
		}

		_MtlInfoList.push_back(mtlInfo);
		_MtlCount++;
	}
}





