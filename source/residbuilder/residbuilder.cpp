// residbuilder.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"
#include "tinyxml/tinyxml.h"
#include <comutil.h>  
#pragma comment(lib, "comsuppw.lib")

const wchar_t  RB_HEADER[] = L"/*<------------------------------------------------------------------------------------------------->*/\n/*���ļ���residbuilder2���ɣ��벻Ҫ�ֶ��޸�*/\n/*<------------------------------------------------------------------------------------------------->*/\n";

const wchar_t RB_RC2INCLUDE[]=L"#pragma once\n#include <duires.h>\n";

string ws2s(const wstring& ws)
{
	_bstr_t t = ws.c_str();
	char* pchar = (char*)t;
	string result = pchar;
	return result;
}

wstring s2ws(const string& s)
{
	_bstr_t t = s.c_str();
	wchar_t* pwchar = (wchar_t*)t;
	wstring result = pwchar;
	return result;
}

struct IDMAPRECORD
{
	WCHAR szType[100];
	WCHAR szName[200];
	WCHAR szPath[MAX_PATH];
};

struct NAME2IDRECORD
{
	WCHAR szName[100];
	int   nID;
	WCHAR szRemark[300];
};

//����ļ�������޸�ʱ��
__int64 GetLastWriteTime(LPCSTR pszFileName)
{
	__int64 tmFile=0;
	WIN32_FIND_DATAA findFileData;
	HANDLE hFind = FindFirstFileA(pszFileName, &findFileData);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		tmFile= *(__int64*)&findFileData.ftLastWriteTime;
		FindClose(hFind);
	}
	return tmFile;
}

//������б��ת����˫��б��
wstring BuildPath(LPCWSTR pszPath)
{
	LPCWSTR p=pszPath;
	WCHAR szBuf[MAX_PATH*2]={0};
	WCHAR *p2=szBuf;
	while(*p)
	{
		if(*p==L'\\')
		{
			if(*(p+1)!=L'\\')
			{//��б��
				p2[0]=p2[1]=L'\\';
				p++;
				p2+=2;
			}else
			{//�Ѿ���˫б��
				p2[0]=p2[1]=L'\\';
				p+=2;
				p2+=2;
			}
		}else
		{
			*p2=*p;
			p++;
			p2++;
		}
	}
	*p2=0;
	return wstring(szBuf);
}

void UpdateName2ID(map<string,int> *pmapName2ID,TiXmlDocument *pXmlDocName2ID,TiXmlElement *pXmlEleLayer,int & nCurID)
{
	const char * strName=pXmlEleLayer->Attribute("name");
	int nID=0;
	pXmlEleLayer->Attribute("id",&nID);
	if(strName)
	{//�ҵ�һ���ڵ���name����
		if(pmapName2ID->find(strName)==pmapName2ID->end())//��ǰname���ڱ���
		{
			TiXmlElement pNewNamedID=TiXmlElement("name2id");
			pNewNamedID.SetAttribute("name",strName);
			if(nID==0) nID=++nCurID;
			pNewNamedID.SetAttribute("id",nID);
			const char * strRemark=pXmlEleLayer->Attribute("fun");
			if(strRemark)
			{
				pNewNamedID.SetAttribute("remark",strRemark);
			}
			pXmlDocName2ID->InsertEndChild(pNewNamedID);
			(*pmapName2ID)[strName]=nID;
		}else
		{
			printf("find a element which uses a used name attribute %s \n",strName);
		}
	}
	TiXmlElement *pXmlChild=pXmlEleLayer->FirstChildElement();
	if(pXmlChild) UpdateName2ID(pmapName2ID,pXmlDocName2ID,pXmlChild,nCurID);
	TiXmlElement *pXmlSibling=pXmlEleLayer->NextSiblingElement();
	if(pXmlSibling) UpdateName2ID(pmapName2ID,pXmlDocName2ID,pXmlSibling,nCurID);
}

#define ID_AUTO_START	65535
#define STAMP_FORMAT	L"//stamp:0000000000000000\r\n"
#define STAMP_FORMAT2	L"//stamp:%08x%08x\r\n"

#pragma pack(push,1)

class FILEHEAD
{
public:
	char szBom[2];
	WCHAR szHeadLine[ARRAYSIZE(STAMP_FORMAT)];

	FILEHEAD(__int64 ts=0)
	{
		szBom[0]=0xFF,szBom[1]=0xFE;
		swprintf(szHeadLine,STAMP_FORMAT2,(ULONG)((ts>>32)&0xffffffff),(ULONG)(ts&0xffffffff));		
	}
	static __int64 ExactTimeStamp(LPCSTR pszFile)
	{
		__int64 ts=0;
		FILE *f=fopen(pszFile,"rb");
		if(f)
		{
			FILEHEAD head;
			fread(&head,sizeof(FILEHEAD),1,f);
			DWORD dHi=0,dLow=0;
			if(wcsncmp(head.szHeadLine,STAMP_FORMAT2,8)==0)
			{
				swscanf(head.szHeadLine,STAMP_FORMAT2,&dHi,&dLow);
				ts=((__int64)dHi)<<32|dLow;
			}
			fclose(f);
		}
		return ts;
	}
};
#pragma  pack(pop)

typedef map<string,int> NAME2IDMAP;

BOOL IsName2IDMapChanged(NAME2IDMAP & map1,NAME2IDMAP & map2)
{
	if(map1.size()!=map2.size()) return TRUE;

	NAME2IDMAP::iterator it=map1.begin();
	while(it!=map1.end())
	{
		NAME2IDMAP::iterator it2=map2.find(it->first);
		if(it2==map2.end()) return TRUE;
		if(it2->second!=it->second) return TRUE;
		it++;
	}
	return FALSE;
}

//residbuilder -y -p skin -i skin\index.xml -r .\duires\winres.rc2 -n .\duires\name2id.xml -h .\duires\winres.h
int _tmain(int argc, _TCHAR* argv[])
{
	string strSkinPath;	//Ƥ��·��,����ڳ����.rc�ļ�
	string strIndexFile;
	string strRes;		//rc2�ļ���
	string strHead;		//��Դͷ�ļ�,��winres.h
	string strName2ID;	//����-IDӳ���XML
	char   cYes=0;		//ǿ�Ƹ�д��־

	int c;

	printf("%s\n",GetCommandLineA());
	while ((c = getopt(argc, argv, _T("i:r:h:n:y:p:"))) != EOF)
	{
		switch (c)
		{
		case _T('i'):strIndexFile=ws2s(optarg);break;
		case _T('r'):strRes=ws2s(optarg);break;
		case _T('h'):strHead=ws2s(optarg);break;
		case _T('n'):strName2ID=ws2s(optarg);break;
		case _T('y'):cYes=1;optind--;break;
		case _T('p'):strSkinPath=ws2s(optarg);break;
		}
	}
	if(strIndexFile.empty())
	{
		printf("not specify input file, using -i to define the input file");
		return 1;
	}

	//��index.xml�ļ�
	TiXmlDocument xmlIndexFile;
	if(!xmlIndexFile.LoadFile(strIndexFile.c_str()))
	{
		printf("parse input file failed");
		return 1;
	}

	vector<IDMAPRECORD> vecIdMapRecord;
	//load xml description of resource to vector
	TiXmlElement *xmlEle=xmlIndexFile.RootElement();
	while(xmlEle)
	{
		if(strcmp(xmlEle->Value(),"resid")==0)
		{
			IDMAPRECORD rec={0};
			const char *pszValue;
			pszValue=xmlEle->Attribute("type");
			if(pszValue) MultiByteToWideChar(CP_UTF8,0,pszValue,-1,rec.szType,100);
			pszValue=xmlEle->Attribute("name");
			if(pszValue) MultiByteToWideChar(CP_UTF8,0,pszValue,-1,rec.szName,200);
			pszValue=xmlEle->Attribute("file");
			if(pszValue)
			{
				string str;
				if(!strSkinPath.empty()){ str=strSkinPath+"\\"+pszValue;}
				else str=pszValue;
				MultiByteToWideChar(CP_UTF8,0,str.c_str(),str.length(),rec.szPath,MAX_PATH);
			}

			vecIdMapRecord.push_back(rec);
		}
		xmlEle=xmlEle->NextSiblingElement();
	}
	if(strRes.length())
	{//������Դ.rc2�ļ�
		//build output string by wide char
		wstring strOut;

		vector<IDMAPRECORD>::iterator it2=vecIdMapRecord.begin();
		while(it2!=vecIdMapRecord.end())
		{
			WCHAR szRec[2000];
			wstring strFile=BuildPath(it2->szPath);
			swprintf(szRec,L"DEFINE_%s(%s,\t%\"%s\")\n",it2->szType,it2->szName,strFile.c_str());
			strOut+=szRec;
			it2++;
		}

		__int64 tmIdx=GetLastWriteTime(strIndexFile.c_str());
		__int64 tmSave=FILEHEAD::ExactTimeStamp(strRes.c_str());
		//write output string to target res file
		if(tmIdx!=tmSave)
		{
			FILE * f=fopen(strRes.c_str(),"wb");
			if(f)
			{
				FILEHEAD tmStamp(tmIdx);
				fwrite(&tmStamp,sizeof(FILEHEAD)-sizeof(WCHAR),1,f);//дUTF16�ļ�ͷ��ʱ�䡣-sizeof(WCHAR)����ȥ��stamp���һ��\0
				fwrite(RB_HEADER,sizeof(WCHAR),wcslen(RB_HEADER),f);
				fwrite(RB_RC2INCLUDE,2,wcslen(RB_RC2INCLUDE),f);
				fwrite(strOut.c_str(),sizeof(WCHAR),strOut.length(),f);
				fclose(f);
				printf("build resource succeed!\n");
			}
		}else
		{
			printf("%s has not been modified\n",strIndexFile.c_str());
		}

	}

	//build resource head
	if(strName2ID.length() && strHead.length())
	{
		TiXmlDocument xmlName2ID;
		xmlName2ID.LoadFile(strName2ID.c_str());
		NAME2IDMAP mapNamedID_Old;
		TiXmlElement *pXmlName2ID=xmlName2ID.FirstChildElement("name2id");
		while(pXmlName2ID)
		{
			string strName=pXmlName2ID->Attribute("name");
			int uID=0;
			pXmlName2ID->Attribute("id",&uID);
			mapNamedID_Old[strName]=uID;
			pXmlName2ID=pXmlName2ID->NextSiblingElement("name2id");
		}
		xmlName2ID.Clear();

		NAME2IDMAP mapNamedID;
		TiXmlElement *pXmlIdmap=xmlIndexFile.FirstChildElement("resid");
		int nCurID=ID_AUTO_START;
		while(pXmlIdmap)
		{
			int layer=0;
			pXmlIdmap->Attribute("layer",&layer);
			if(layer && _stricmp(pXmlIdmap->Attribute("type"),"xml")==0)
			{
				string strXmlLayer=pXmlIdmap->Attribute("file");
				if(!strSkinPath.empty()) strXmlLayer=strSkinPath+"\\"+strXmlLayer;
				if(strXmlLayer.length())
				{//�ҵ�һ����������XML
					printf("extracting named element from %s\n",strXmlLayer.c_str());
					TiXmlDocument xmlDocLayer;
					xmlDocLayer.LoadFile(strXmlLayer.c_str());
					UpdateName2ID(&mapNamedID,&xmlName2ID,xmlDocLayer.RootElement(),nCurID);
				}
			}
			pXmlIdmap=pXmlIdmap->NextSiblingElement("resid");
		}

		if(!IsName2IDMapChanged(mapNamedID,mapNamedID_Old))
		{
			printf("name2id map doesn't need to be updated!");
		}else
		{

			FILE *f=fopen(strName2ID.c_str(),"w");
			if(f)
			{
				xmlName2ID.Print(f);
				fclose(f);
				printf("build name2id succeed!");
			}

			vector<NAME2IDRECORD> vecName2ID;
			//load xml description of resource to vector
			TiXmlElement *xmlEle=xmlName2ID.RootElement();
			while(xmlEle)
			{
				if(strcmp(xmlEle->Value(),"name2id")==0)
				{
					NAME2IDRECORD rec={0};
					const char *pszValue;
					pszValue=xmlEle->Attribute("name");
					if(pszValue) MultiByteToWideChar(CP_UTF8,0,pszValue,-1,rec.szName,100);
					pszValue=xmlEle->Attribute("id");
					if(pszValue) rec.nID=atoi(pszValue);
					pszValue=xmlEle->Attribute("remark");
					if(pszValue) MultiByteToWideChar(CP_UTF8,0,pszValue,-1,rec.szRemark,300);

					if(rec.szName[0] && rec.nID) vecName2ID.push_back(rec);
				}
				xmlEle=xmlEle->NextSiblingElement();
			}

			//build output string by wide char
			wstring strOut;
			strOut+=RB_HEADER;

			vector<NAME2IDRECORD>::iterator it2=vecName2ID.begin();
			while(it2!=vecName2ID.end())
			{
				WCHAR szRec[2000];
				if(it2->szRemark[0])
					swprintf(szRec,L"#define\t%s\t\t%d	\t//%s\n",it2->szName,it2->nID,it2->szRemark);
				else
					swprintf(szRec,L"#define\t%s\t\t%d\n",it2->szName,it2->nID);
				strOut+=szRec;
				it2++;
			}

			//write output string to target res file
			f=fopen(strHead.c_str(),"wb");
			if(f)
			{
				char szBom[2]={0xFF,0xFE};
				fwrite(szBom,2,1,f);//дUTF16�ļ�ͷ��
				fwrite(strOut.c_str(),sizeof(WCHAR),strOut.length(),f);
				fclose(f);
				printf("build header succeed!\n");
			}
		}
	}

	return 0;
}

