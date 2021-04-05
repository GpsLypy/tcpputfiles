#include "_freecplus.h"

//定义存储信息的数据结构

struct st_arg
{
  char ip[31];              //服务器端的ip地址
  int  port;               //服务器端的端口
  int  ptype;               // 文件发送成功后文件的处理方式：1-保留文件；2-删除文件；3-移动到备份目录。
  char clientpath[301];     // 本地文件存放的根目录。
  char clientpathbak[301];  // 文件成功发送后，本地文件备份的根目录，当ptype==3时有效。
  char srvpath[301];        // 服务端文件存放的根目录。
  bool andchild;            // 是否发送clientpath目录下各级子目录的文件，true-是；false-否。
  char matchname[301];      // 待发送文件名的匹配方式，如"*.TXT,*.XML"，注意用大写。
  char okfilename[301];     // 已发送成功文件名清单。
  int  timetvl;             // 扫描本地目录文件的时间间隔，单位：秒。
} starg;

char strRecvBuffer[1024]; // 接收报文的缓冲区
char strSendBuffer[1024]; // 发送报文的缓冲区

CTcpClient TcpClient;

CLogFile logfile;

//程序的帮助函数
void _help();

// 把参数xml解析到starg结构中
bool _xmltoarg(char *strxmlbuffer);

// 客户端登录服务器
bool ClientLogin(const char *argv);

// 向服务端发送心跳报文
bool ActiveTest();

// 实现文件发送的功能
bool _tcpputfiles();

vector<struct st_fileinfo> vlistfile,vlistfile1;
vector<struct st_fileinfo> vokfilename,vokfilename1;

// 把clientpath目录下的文件加载到vlistfile容器中
bool LoadListFile();

//把okfilename文件内容加载到vokfilename容器中
bool LoadOKFileName();
//
// 把vlistfile容器中的文件与vokfilename容器中文件对比，得到两个容器
// 一、在vlistfile中存在，并已经采集成功的文件vokfilename1
//二、在vlistfile中存在，新文件或需要重新采集的文件vlistfile1
bool CompVector();


//把vokfilename1容器中的内容先写入okfilename文件中，覆盖之前的旧okfilename文件
bool WriteToOKFileName();

// 如果ptype==1，把采集成功的文件记录追加到okfilename文件中
bool AppendToOKFileName(struct st_fileinfo *stfileinfo);

void EXIT(int sig);


int main(int argc,char *argv[])
{
  if(argc!=3) {_help();return -1;}
  // 关闭全部的信号和输入输出
  CloseIOAndSignal();
  // 处理程序退出的信号
  signal(SIGINT,EXIT);signal(SIGTERM,EXIT);
 
  if (logfile.Open(argv[1],"a+")==false)
  {
    printf("打开日志文件失败（%s）。\n",argv[1]); return -1;
  } 
 
   // 把xml解析到参数starg结构中
  if (_xmltoarg(argv[2])==false) return -1;
  
   while (true)  //在while循环里面，如果通信链路出现问题（文件发送失败或心跳失效），可以实现客户端重连机制。
  {             
    // 向服务器发起连接并登录,此功能函数必须保证一直尝试连接(间隔建议每十秒尝试登录服务器)直到成功登录
    ClientLogin(argv[2]);

    // 实现文件发送的功能
    _tcpputfiles();

    if (vlistfile.size()==0)
    {
      // 向服务端发送心跳报文
      ActiveTest(); 
    
      sleep(starg.timetvl);
    }
  }

  return 0;
  
}
void EXIT(int sig)
{
  logfile.Write("程序退出，sig=%d\n\n",sig);

  TcpClient.Close();

  exit(0);
}

void _help()
{
  printf("\n");
  printf("Using:/htidc/public/bin/tcpputfiles logfilename xmlbuffer\n\n");

  printf("Sample:/htidc/public/bin/tcpputfiles /log/shqx/tcpputfiles_surfdata.log \"<ip>172.16.0.15</ip><port>5010</port><ptype>1</ptype><clientpath>/data/shqx/sdata/surfdata</clientpath><clientpathbak>/data/shqx/sdata/surfdatabak</clientpathbak><srvpath>/data/shqx/tcp/surfdata</srvpath><andchild>true</andchild><matchname>SURF_*.TXT,*.DAT</matchname><okfilename>/data/shqx/tcplist/tcpputfiles_surfdata.xml</okfilename><timetvl>10</timetvl>\"\n\n\n");

  printf("本程序采用TCP协议把文件发送给服务端。\n");
  printf("logfilename  本程序运行的日志文件。\n");
  printf("xmlbuffer    本程序运行的参数，如下：\n");
  printf("ip           服务器端的IP地址。\n");
  printf("port         服务器端的端口。\n");
  printf("ptype        文件发送成功后的处理方式：1-保留文件；2-删除文件；3-移动到备份目录。\n");
  printf("clientpath    本地文件存放的根目录。\n");
  printf("clientpathbak 文件成功发送后，本地文件备份的根目录，当ptype==3时有效，缺省为空。\n");
  printf("srvpath      服务端文件存放的根目录。\n");
  printf("andchild     是否发送clientpath目录下各级子目录的文件，true-是；false-否，缺省为false。\n");
  printf("matchname    待发送文件名的匹配方式，如\"*.TXT,*.XML\"，注意用大写。\n");
  printf("okfilename   已发送成功文件名清单，缺省为空。\n");
  printf("timetvl      扫描本地目录文件的时间间隔，单位：秒，取值在2-50之间。\n\n\n");
 
}

// 把参数xml解析到starg结构中
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

// 登录服务器
bool ClientLogin(const char *argv)
{
  if(TcpClient.m_sockfd>0) return true;//如果sockf>0,说明已经连接上服务器了

  int ii=0;

  while(true)
   {
     if(ii++>0) sleep(20);    // 第一次进入循环不休眠
      // 向服务器发起连接
     if (TcpClient.ConnectToServer(starg.ip,starg.port) == false)
     {
       logfile.Write("TcpClient.ConnectToServer(%s,%d) failed.\n",starg.ip,starg.port); continue;
     }

     memset(strRecvBuffer,0,sizeof(strRecvBuffer));
     memset(strSendBuffer,0,sizeof(strSendBuffer));
     //把运行参数复制到发送缓冲区，再接上客户端业务请求
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

// 向服务端发送心跳报文
bool ActiveTest()
{
     memset(strRecvBuffer,0,sizeof(strRecvBuffer));
     memset(strSendBuffer,0,sizeof(strSendBuffer));
	 
     strcpy(strSendBuffer,"<activetest>ok</activetest>");
     // logfile.Write("strSendBuffer=%s\n",strSendBuffer);  // xxxxxx
	 //Write 和 read函数出错概率很小，如果出错1大概率说明通信链路出现问题，网络不可用，需要重新连接服务器
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

// 实现文件发送的功能
bool _tcpputfiles()
{
  //扫描本地目录(clientpath)，并把扫描当次目录下的文件加载到vlistfile容器中，此功能由LoadListFile函数实现
  if (LoadListFile()==false)
  {
    logfile.Write("LoadListFile() failed.\n"); return false;
  }
  if(starg.ptype==1)
  {
	// 加载okfilename文件中的内容到容器vokfilename中
    LoadOKFileName();

    // 把vlistfile容器中的文件与vokfilename容器中文件对比，得到两个容器
    // 一、在vlistfile中存在，并已经采集成功的文件vokfilename1
    // 二、在vlistfile中存在，新文件或需要重新采集的文件vlistfile1********
    CompVector();

    // 把vokfilename1容器中的内容先写入okfilename文件中，覆盖之前的旧okfilename文件
    WriteToOKFileName();
   
    // 把vlistfile1容器中的内容复制到vlistfile容器中
    vlistfile.clear(); vlistfile.swap(vlistfile1);
  }
  //开始发送容器内所有文件
  for(int ii=0;ii<vlistfile.size();ii++)
  {
	logfile.Write("put %s ...",vlistfile[ii].filename);
	// 把文件发送给服务端
    // if (SendFile(&logfile,TcpClient.m_sockfd,&vlistfile[ii])==false) 
	// 把文件通过sockfd发送给对端
    if(SendFile(TcpClient.m_sockfd,&vlistfile[ii],&logfile)==false)
    {
      logfile.Write("RecvFile() failed.\n"); TcpClient.Close(); return false;
    }

    logfile.WriteEx("ok.\n");
	// 删除文件
    if(starg.ptype==2)  REMOVE(vlistfile[ii].filename);
    // 转存到备份目录
    if(starg.ptype==3) 
    {
      char strfilenamebak[301];
      memset(strfilenamebak,0,sizeof(strfilenamebak));
      strcpy(strfilenamebak,vlistfile[ii].filename);
	  
      // 字符串替换函数。在字符串str中，如果存在字符串str1，就替换为字符串str2。
      // str：待处理的字符串, str1：旧的内容,str2：新的内容。
      // bloop：是否循环执行替换。
      // 注意：
      // 1、如果str2比str1要长，替换后str会变长，所以必须保证str有足够的空间，否则内存会溢出。
      // 2、如果str2中包含了str1的内容，且bloop为true，这种做法存在逻辑错误，UpdateStr将什么也不做。
      //void UpdateStr(char *str,const char *str1,const char *str2,const bool bloop=true);
      //str:/data/shqx/sdata/surfdata/text.jpg; str1:/data/shqx/sdata/surfdata; str2:/data/shqx/sdata/surfdatabak
      UpdateStr(strfilenamebak,starg.clientpath,starg.clientpathbak,false);  // 要小心第三个参数
	  
      if (RENAME(vlistfile[ii].filename,strfilenamebak)==false)//测试删掉源文件，备份文件是否有效，验证rename函数是否会进行数据复制
      {
        logfile.Write("RENAME %s to %s failed.\n",vlistfile[ii].filename,strfilenamebak); return false;
      }
	}
	// 如果ptype==1，把发送成功的文件记录追加到okfilename文件中
	if(starg.ptype==1) AppendToOKFileName(&vlistfile[ii]);
  }
  return true;
}

// 把clientpath目录下的文件加载到vlistfile容器中(vector<struct st_fileinfo> vlistfile,vlistfile1;)
bool LoadListFile()
{
  //清空vlistfile容器(容器本身就是一个类)
  vlistfile.clear();
  //实例化一个用来操作目录类的对象
  CDir Dir;
  // 打开目录，获取目录中的文件列表信息，存放于m_vFileName容器中(存放OpenDir方法获取到的绝对路径文件名清单。)。
  // in_DirName，待打开的目录名，采用绝对路径，如/tmp/root。
  // in_MatchStr，待获取文件名的匹配规则，不匹配的文件被忽略，具体请参见freecplus框架的MatchStr函数。
  // in_MaxCount，获取文件的最大数量，缺省值为10000个。
  // bAndChild，是否打开各级子目录，缺省值为false-不打开子目录。
  // bSort，是否对获取到的文件列表（即m_vFileName容器中的内容）进行排序，缺省值为false-不排序。
  // 返回值：true-成功，false-失败，如果in_DirName参数指定的目录不存在，OpenDir方法会创建该目录，如果创建失败，返回false，如果当前用户对in_DirName目录下的子目录没有读取权限也会返回false。
  // 注意，如果目录下的总文件数超过50000，增量发送文件功能将有问题
  if( Dir.OpenDir(starg.clientpath,starg.matchname,50000,starg.andchild,false)==false)
  {
	logfile.Write("Dir.OpenDir(%s) 失败。\n",starg.clientpath); return false;
  }
  struct st_fileinfo stfileinfo;
  
  while(true)
  {
    memset(&stfileinfo,0,sizeof(struct st_fileinfo));
  
    // 从m_vFileName容器中获取一条记录（文件名），同时获取该文件的大小、修改时间等信息。
    // 调用OpenDir方法时，m_vFileName容器被填充，m_pos归零，每调用一次ReadDir方法m_pos加1。
    // 当m_pos小于m_vFileName.size()，返回true，否则返回false。
    if (Dir.ReadDir()==false) break;

    strcpy(stfileinfo.filename,Dir.m_FullFileName);
    stfileinfo.filesize=Dir.m_FileSize;
    strcpy(stfileinfo.mtime,Dir.m_ModifyTime);
  
    vlistfile.push_back(stfileinfo);
	// logfile.Write("vlistfile filename=%s,mtime=%s\n",stfileinfo.filename,stfileinfo.mtime);
  }
  return true;
}

// 把okfilename文件内容加载到vokfilename容器中
bool LoadOKFileName()
{
  vokfilename.clear();
  char strfileinfo[301];
  
  struct st_fileinfo stfileinfo;
  CFile File;
  
  // 注意：如果程序是第一次采集，okfilename是不存在的，并不是错误，所以也返回true。
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

// 把vlistfile容器中的文件与vokfilename容器中文件对比，得到两个容器
// // 一、在vlistfile中存在，并已经采集成功的文件vokfilename1
// // 二、在vlistfile中存在，新文件或需要重新采集的文件vlistfile1
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
	    vokfilename1.push_back(vlistfile[ii]);//两个容器有相同的数据结构
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

// 把vokfilename1容器中的内容先写入okfilename文件中，覆盖之前的旧okfilename文件
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

// 如果ptype==1，把采集成功的文件记录追加到okfilename文件中
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




