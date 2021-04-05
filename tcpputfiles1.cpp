#include "_freecplus.h"

//����洢��Ϣ�����ݽṹ

struct st_arg
{
  char ip[31];              //�������˵�ip��ַ
  int  port;               //�������˵Ķ˿�
  int  ptype;               // �ļ����ͳɹ����ļ��Ĵ���ʽ��1-�����ļ���2-ɾ���ļ���3-�ƶ�������Ŀ¼��
  char clientpath[301];     // �����ļ���ŵĸ�Ŀ¼��
  char clientpathbak[301];  // �ļ��ɹ����ͺ󣬱����ļ����ݵĸ�Ŀ¼����ptype==3ʱ��Ч��
  char srvpath[301];        // ������ļ���ŵĸ�Ŀ¼��
  bool andchild;            // �Ƿ���clientpathĿ¼�¸�����Ŀ¼���ļ���true-�ǣ�false-��
  char matchname[301];      // �������ļ�����ƥ�䷽ʽ����"*.TXT,*.XML"��ע���ô�д��
  char okfilename[301];     // �ѷ��ͳɹ��ļ����嵥��
  int  timetvl;             // ɨ�豾��Ŀ¼�ļ���ʱ��������λ���롣
} starg;

char strRecvBuffer[1024]; // ���ձ��ĵĻ�����
char strSendBuffer[1024]; // ���ͱ��ĵĻ�����

CTcpClient TcpClient;

CLogFile logfile;

//����İ�������
void _help();

// �Ѳ���xml������starg�ṹ��
bool _xmltoarg(char *strxmlbuffer);

// �ͻ��˵�¼������
bool ClientLogin(const char *argv);

// �����˷�����������
bool ActiveTest();

// ʵ���ļ����͵Ĺ���
bool _tcpputfiles();

vector<struct st_fileinfo> vlistfile,vlistfile1;
vector<struct st_fileinfo> vokfilename,vokfilename1;

// ��clientpathĿ¼�µ��ļ����ص�vlistfile������
bool LoadListFile();

//��okfilename�ļ����ݼ��ص�vokfilename������
bool LoadOKFileName();
//
// ��vlistfile�����е��ļ���vokfilename�������ļ��Աȣ��õ���������
// һ����vlistfile�д��ڣ����Ѿ��ɼ��ɹ����ļ�vokfilename1
//������vlistfile�д��ڣ����ļ�����Ҫ���²ɼ����ļ�vlistfile1
bool CompVector();


//��vokfilename1�����е�������д��okfilename�ļ��У�����֮ǰ�ľ�okfilename�ļ�
bool WriteToOKFileName();

// ���ptype==1���Ѳɼ��ɹ����ļ���¼׷�ӵ�okfilename�ļ���
bool AppendToOKFileName(struct st_fileinfo *stfileinfo);

void EXIT(int sig);


int main(int argc,char *argv[])
{
  if(argc!=3) {_help();return -1;}
  // �ر�ȫ�����źź��������
  CloseIOAndSignal();
  // ��������˳����ź�
  signal(SIGINT,EXIT);signal(SIGTERM,EXIT);
 
  if (logfile.Open(argv[1],"a+")==false)
  {
    printf("����־�ļ�ʧ�ܣ�%s����\n",argv[1]); return -1;
  } 
 
   // ��xml����������starg�ṹ��
  if (_xmltoarg(argv[2])==false) return -1;
  
   while (true)  //��whileѭ�����棬���ͨ����·�������⣨�ļ�����ʧ�ܻ�����ʧЧ��������ʵ�ֿͻ����������ơ�
  {             
    // ��������������Ӳ���¼,�˹��ܺ������뱣֤һֱ��������(�������ÿʮ�볢�Ե�¼������)ֱ���ɹ���¼
    ClientLogin(argv[2]);

    // ʵ���ļ����͵Ĺ���
    _tcpputfiles();

    if (vlistfile.size()==0)
    {
      // �����˷�����������
      ActiveTest(); 
    
      sleep(starg.timetvl);
    }
  }

  return 0;
  
}
void EXIT(int sig)
{
  logfile.Write("�����˳���sig=%d\n\n",sig);

  TcpClient.Close();

  exit(0);
}

void _help()
{
  printf("\n");
  printf("Using:/htidc/public/bin/tcpputfiles logfilename xmlbuffer\n\n");

  printf("Sample:/htidc/public/bin/tcpputfiles /log/shqx/tcpputfiles_surfdata.log \"<ip>172.16.0.15</ip><port>5010</port><ptype>1</ptype><clientpath>/data/shqx/sdata/surfdata</clientpath><clientpathbak>/data/shqx/sdata/surfdatabak</clientpathbak><srvpath>/data/shqx/tcp/surfdata</srvpath><andchild>true</andchild><matchname>SURF_*.TXT,*.DAT</matchname><okfilename>/data/shqx/tcplist/tcpputfiles_surfdata.xml</okfilename><timetvl>10</timetvl>\"\n\n\n");

  printf("���������TCPЭ����ļ����͸�����ˡ�\n");
  printf("logfilename  ���������е���־�ļ���\n");
  printf("xmlbuffer    ���������еĲ��������£�\n");
  printf("ip           �������˵�IP��ַ��\n");
  printf("port         �������˵Ķ˿ڡ�\n");
  printf("ptype        �ļ����ͳɹ���Ĵ���ʽ��1-�����ļ���2-ɾ���ļ���3-�ƶ�������Ŀ¼��\n");
  printf("clientpath    �����ļ���ŵĸ�Ŀ¼��\n");
  printf("clientpathbak �ļ��ɹ����ͺ󣬱����ļ����ݵĸ�Ŀ¼����ptype==3ʱ��Ч��ȱʡΪ�ա�\n");
  printf("srvpath      ������ļ���ŵĸ�Ŀ¼��\n");
  printf("andchild     �Ƿ���clientpathĿ¼�¸�����Ŀ¼���ļ���true-�ǣ�false-��ȱʡΪfalse��\n");
  printf("matchname    �������ļ�����ƥ�䷽ʽ����\"*.TXT,*.XML\"��ע���ô�д��\n");
  printf("okfilename   �ѷ��ͳɹ��ļ����嵥��ȱʡΪ�ա�\n");
  printf("timetvl      ɨ�豾��Ŀ¼�ļ���ʱ��������λ���룬ȡֵ��2-50֮�䡣\n\n\n");
 
}

// �Ѳ���xml������starg�ṹ��
bool _xmltoarg(char *strxmlbuffer)
{
  memset(&starg,0,sizeof(struct st_arg));

  GetXMLBuffer(strxmlbuffer,"ip",starg.ip);
  if (strlen(starg.ip)==0) { logfile.Write("ip is null.\n"); return false; }

  GetXMLBuffer(strxmlbuffer,"port",&starg.port);
  if ( starg.port==0) { logfile.Write("port is null.\n"); return false; }

  GetXMLBuffer(strxmlbuffer,"ptype",&starg.ptype);
  if ((starg.ptype!=1)&&(starg.ptype!=2)&&(starg.ptype!=3) ) { logfile.Write("ptype not in (1,2,3).\n"); return false; }

  GetXMLBuffer(strxmlbuffer,"clientpath",starg.clientpath);
  if (strlen(starg.clientpath)==0) { logfile.Write("clientpath is null.\n"); return false; }

  GetXMLBuffer(strxmlbuffer,"clientpathbak",starg.clientpathbak);
  if ((starg.ptype==3)&&(strlen(starg.clientpathbak)==0)) { logfile.Write("clientpathbak is null.\n"); return false; }

  GetXMLBuffer(strxmlbuffer,"srvpath",starg.srvpath);
  if (strlen(starg.srvpath)==0) { logfile.Write("srvpath is null.\n"); return false; }

  GetXMLBuffer(strxmlbuffer,"andchild",&starg.andchild);
  
   GetXMLBuffer(strxmlbuffer,"matchname",starg.matchname);
  if (strlen(starg.matchname)==0) { logfile.Write("matchname is null.\n"); return false; }

  GetXMLBuffer(strxmlbuffer,"okfilename",starg.okfilename);
  if ((starg.ptype==1)&&(strlen(starg.okfilename)==0)) { logfile.Write("okfilename is null.\n"); return false; }

  GetXMLBuffer(strxmlbuffer,"timetvl",&starg.timetvl);
  if (starg.timetvl==0) { logfile.Write("timetvl is null.\n"); return false; }

  if (starg.timetvl>50) starg.timetvl=50;

  return true;
}

// ��¼������
bool ClientLogin(const char *argv)
{
  if(TcpClient.m_sockfd>0) return true;//���sockf>0,˵���Ѿ������Ϸ�������

  int ii=0;

  while(true)
   {
     if(ii++>0) sleep(20);    // ��һ�ν���ѭ��������
      // ���������������
     if (TcpClient.ConnectToServer(starg.ip,starg.port) == false)
     {
       logfile.Write("TcpClient.ConnectToServer(%s,%d) failed.\n",starg.ip,starg.port); continue;
     }

     memset(strRecvBuffer,0,sizeof(strRecvBuffer));
     memset(strSendBuffer,0,sizeof(strSendBuffer));
     //�����в������Ƶ����ͻ��������ٽ��Ͽͻ���ҵ������
     strcpy(strSendBuffer,argv); strcat(strSendBuffer,"<clienttype>1</clienttype>");
     // logfile.Write("strSendBuffer=%s\n",strSendBuffer);  // xxxxxx
     if (TcpClient.Write(strSendBuffer) == false)
     {
       logfile.Write("1 TcpClient.Write() failed.\n"); continue;
     }

     if (TcpClient.Read(strRecvBuffer,20) == false)
     {
       logfile.Write("1 TcpClient.Read() failed.\n"); continue;
     }
     // logfile.Write("strRecvBuffer=%s\n",strRecvBuffer);  // xxxxxx

     break;
   }

   logfile.Write("login(%s,%d) ok.\n",starg.ip,starg.port);

   return true;
}

// �����˷�����������
bool ActiveTest()
{
     memset(strRecvBuffer,0,sizeof(strRecvBuffer));
     memset(strSendBuffer,0,sizeof(strSendBuffer));
	 
     strcpy(strSendBuffer,"<activetest>ok</activetest>");
     // logfile.Write("strSendBuffer=%s\n",strSendBuffer);  // xxxxxx
	 //Write �� read����������ʺ�С���������1�����˵��ͨ����·�������⣬���粻���ã���Ҫ�������ӷ�����
     if (TcpClient.Write(strSendBuffer) == false)
     {
       logfile.Write("2 TcpClient.Write() failed.\n");TcpClient.Close(); return false;
     }

     if (TcpClient.Read(strRecvBuffer,6) == false)
     {
       logfile.Write("2 TcpClient.Read() failed.\n"); TcpClient.Close(); return false;
     }
	  
	 if( strcmp(strRecvBuffer,"ok")!=0 ) {TcpClient.Close(); return false;} 
	 return true;
	
}

// ʵ���ļ����͵Ĺ���
bool _tcpputfiles()
{
  //ɨ�豾��Ŀ¼(clientpath)������ɨ�赱��Ŀ¼�µ��ļ����ص�vlistfile�����У��˹�����LoadListFile����ʵ��
  if (LoadListFile()==false)
  {
    logfile.Write("LoadListFile() failed.\n"); return false;
  }
  if(starg.ptype==1)
  {
	// ����okfilename�ļ��е����ݵ�����vokfilename��
    LoadOKFileName();

    // ��vlistfile�����е��ļ���vokfilename�������ļ��Աȣ��õ���������
    // һ����vlistfile�д��ڣ����Ѿ��ɼ��ɹ����ļ�vokfilename1
    // ������vlistfile�д��ڣ����ļ�����Ҫ���²ɼ����ļ�vlistfile1********
    CompVector();

    // ��vokfilename1�����е�������д��okfilename�ļ��У�����֮ǰ�ľ�okfilename�ļ�
    WriteToOKFileName();
   
    // ��vlistfile1�����е����ݸ��Ƶ�vlistfile������
    vlistfile.clear(); vlistfile.swap(vlistfile1);
  }
  //��ʼ���������������ļ�
  for(int ii=0;ii<vlistfile.size();ii++)
  {
	logfile.Write("put %s ...",vlistfile[ii].filename);
	// ���ļ����͸������
    // if (SendFile(&logfile,TcpClient.m_sockfd,&vlistfile[ii])==false) 
	// ���ļ�ͨ��sockfd���͸��Զ�
    if(SendFile(TcpClient.m_sockfd,&vlistfile[ii],&logfile)==false)
    {
      logfile.Write("RecvFile() failed.\n"); TcpClient.Close(); return false;
    }

    logfile.WriteEx("ok.\n");
	// ɾ���ļ�
    if(starg.ptype==2)  REMOVE(vlistfile[ii].filename);
    // ת�浽����Ŀ¼
    if(starg.ptype==3) 
    {
      char strfilenamebak[301];
      memset(strfilenamebak,0,sizeof(strfilenamebak));
      strcpy(strfilenamebak,vlistfile[ii].filename);
	  
      // �ַ����滻���������ַ���str�У���������ַ���str1�����滻Ϊ�ַ���str2��
      // str����������ַ���, str1���ɵ�����,str2���µ����ݡ�
      // bloop���Ƿ�ѭ��ִ���滻��
      // ע�⣺
      // 1�����str2��str1Ҫ�����滻��str��䳤�����Ա��뱣֤str���㹻�Ŀռ䣬�����ڴ�������
      // 2�����str2�а�����str1�����ݣ���bloopΪtrue���������������߼�����UpdateStr��ʲôҲ������
      //void UpdateStr(char *str,const char *str1,const char *str2,const bool bloop=true);
      //str:/data/shqx/sdata/surfdata/text.jpg; str1:/data/shqx/sdata/surfdata; str2:/data/shqx/sdata/surfdatabak
      UpdateStr(strfilenamebak,starg.clientpath,starg.clientpathbak,false);  // ҪС�ĵ���������
	  
      if (RENAME(vlistfile[ii].filename,strfilenamebak)==false)//����ɾ��Դ�ļ��������ļ��Ƿ���Ч����֤rename�����Ƿ��������ݸ���
      {
        logfile.Write("RENAME %s to %s failed.\n",vlistfile[ii].filename,strfilenamebak); return false;
      }
	}
	// ���ptype==1���ѷ��ͳɹ����ļ���¼׷�ӵ�okfilename�ļ���
	if(starg.ptype==1) AppendToOKFileName(&vlistfile[ii]);
  }
  return true;
}

// ��clientpathĿ¼�µ��ļ����ص�vlistfile������(vector<struct st_fileinfo> vlistfile,vlistfile1;)
bool LoadListFile()
{
  //���vlistfile����(�����������һ����)
  vlistfile.clear();
  //ʵ����һ����������Ŀ¼��Ķ���
  CDir Dir;
  // ��Ŀ¼����ȡĿ¼�е��ļ��б���Ϣ�������m_vFileName������(���OpenDir������ȡ���ľ���·���ļ����嵥��)��
  // in_DirName�����򿪵�Ŀ¼�������þ���·������/tmp/root��
  // in_MatchStr������ȡ�ļ�����ƥ����򣬲�ƥ����ļ������ԣ�������μ�freecplus��ܵ�MatchStr������
  // in_MaxCount����ȡ�ļ������������ȱʡֵΪ10000����
  // bAndChild���Ƿ�򿪸�����Ŀ¼��ȱʡֵΪfalse-������Ŀ¼��
  // bSort���Ƿ�Ի�ȡ�����ļ��б���m_vFileName�����е����ݣ���������ȱʡֵΪfalse-������
  // ����ֵ��true-�ɹ���false-ʧ�ܣ����in_DirName����ָ����Ŀ¼�����ڣ�OpenDir�����ᴴ����Ŀ¼���������ʧ�ܣ�����false�������ǰ�û���in_DirNameĿ¼�µ���Ŀ¼û�ж�ȡȨ��Ҳ�᷵��false��
  // ע�⣬���Ŀ¼�µ����ļ�������50000�����������ļ����ܽ�������
  if( Dir.OpenDir(starg.clientpath,starg.matchname,50000,starg.andchild,false)==false)
  {
	logfile.Write("Dir.OpenDir(%s) ʧ�ܡ�\n",starg.clientpath); return false;
  }
  struct st_fileinfo stfileinfo;
  
  while(true)
  {
    memset(&stfileinfo,0,sizeof(struct st_fileinfo));
  
    // ��m_vFileName�����л�ȡһ����¼���ļ�������ͬʱ��ȡ���ļ��Ĵ�С���޸�ʱ�����Ϣ��
    // ����OpenDir����ʱ��m_vFileName��������䣬m_pos���㣬ÿ����һ��ReadDir����m_pos��1��
    // ��m_posС��m_vFileName.size()������true�����򷵻�false��
    if (Dir.ReadDir()==false) break;

    strcpy(stfileinfo.filename,Dir.m_FullFileName);
    stfileinfo.filesize=Dir.m_FileSize;
    strcpy(stfileinfo.mtime,Dir.m_ModifyTime);
  
    vlistfile.push_back(stfileinfo);
	// logfile.Write("vlistfile filename=%s,mtime=%s\n",stfileinfo.filename,stfileinfo.mtime);
  }
  return true;
}

// ��okfilename�ļ����ݼ��ص�vokfilename������
bool LoadOKFileName()
{
  vokfilename.clear();
  char strfileinfo[301];
  
  struct st_fileinfo stfileinfo;
  CFile File;
  
  // ע�⣺��������ǵ�һ�βɼ���okfilename�ǲ����ڵģ������Ǵ�������Ҳ����true��
  if (File.Open(starg.okfilename,"r") == false) return true;
  
  while(true)
  { 
    memset(&stfileinfo,0,sizeof(stfileinfo));
    if(File.Fgets(strfileinfo,300,false)==false) break;
    GetXMLBuffer(strfileinfo,"filename",stfileinfo.filename,300);
    GetXMLBuffer(strfileinfo,"mtime",stfileinfo.mtime,20);

    vokfilename.push_back(stfileinfo);
   // logfile.Write("vokfilename filename=%s,mtime=%s\n",stfileinfo.filename,stfileinfo.mtime);
  }
 
  return true;
  
}

// ��vlistfile�����е��ļ���vokfilename�������ļ��Աȣ��õ���������
// // һ����vlistfile�д��ڣ����Ѿ��ɼ��ɹ����ļ�vokfilename1
// // ������vlistfile�д��ڣ����ļ�����Ҫ���²ɼ����ļ�vlistfile1
bool CompVector()
{
 vokfilename1.clear();  vlistfile1.clear();
 
 for(int ii=0;ii<vlistfile.size();ii++)
 {
    int jj=0;  
    for (jj=0;jj<vokfilename.size();jj++)
	{
	  if( (strcmp(vlistfile[ii].filename,vokfilename[jj].filename)==0)&&(strcmp(vlistfile[ii].mtime,vokfilename[jj].mtime)==0) )
	  {
	    vokfilename1.push_back(vlistfile[ii]);//������������ͬ�����ݽṹ
		break;
	  }
	}
	 
	if(jj==vokfilename.size())
	{
      vlistfile1.push_back(vlistfile[ii]);
    }
 }

 return true; 
}

// ��vokfilename1�����е�������д��okfilename�ļ��У�����֮ǰ�ľ�okfilename�ļ�
 bool WriteToOKFileName()
{
  CFile File;
  if(File.Open(starg.okfilename,"w")==false)
  {
    logfile.Write("File.Open(%s) failed.\n",starg.okfilename); return false;
  }
  for(int ii=0;ii<vokfilename1.size();ii++)
  {
   File.Fprintf("<filename>%s</filename><mtime>%s</mtime>\n",vokfilename1[ii].filename,vokfilename1[ii].mtime);
  }
  return true;
}

// ���ptype==1���Ѳɼ��ɹ����ļ���¼׷�ӵ�okfilename�ļ���
//bool AppendToOKFileName(struct st_fileinfo *stfileinfo)
bool AppendToOKFileName(struct st_fileinfo *stfileinfo)
{
  CFile File;
  
  if(File.Open(starg.okfilename,"a")==false)
  {
    logfile.Write("File.Open(%s) failed.\n",starg.okfilename); return false;
  }

  //File.Fprintf("<filename>%s</filename><mtime>%s</mtime>\n",stfileinfo->filename,stfileinfo->mtime);
  File.Fprintf("<filename>%s</filename><mtime>%s</mtime>\n",stfileinfo->filename,stfileinfo->mtime);
  return true;
	
}




