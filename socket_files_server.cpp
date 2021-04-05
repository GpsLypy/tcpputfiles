//
// 本程序演示采用CTcpServer类，实现socket通讯的服务端和文件传输。
// 

#include "_public.h"

// 接收文件的内容
bool RecvFile(char *strRecvBuffer,int sockfd,char *strfilename);

int main(int argc,char *argv[])
{
  if (argc != 3)
  {
    printf("\n");
    printf("Using:./demo14 port filename\n\n");

    printf("Example:./demo14 5010 test2.jpg\n\n");

    printf("本程序演示采用CTcpServer类，实现socket通讯的服务端和文件传输。\n\n");

    return -1;
  }

  CTcpServer TcpServer;

  // 服务端初始化
  if (TcpServer.InitServer(atoi(argv[1])) == false)
  {
    printf("TcpServer.InitServer(%s) failed.\n",argv[1]); return -1;
  }

  // 等待客户端的连接
  if (TcpServer.Accept() == false)
  {
    printf("TcpServer.Accept() failed.\n"); return -1;
  }

  char strRecvBuffer[1024],strSendBuffer[1024];

  memset(strRecvBuffer,0,sizeof(strRecvBuffer));

  // 读取客户端的报文，等时间是20秒
  if (TcpServer.Read(strRecvBuffer,20)==false) 
  {
    printf("TcpServer.Read() failed.\n"); return -1;
  }

  printf("recv:%s\n",strRecvBuffer);

  printf("recv file ...");

  memset(strSendBuffer,0,sizeof(strSendBuffer));

  // 接收文件的内容
  if (RecvFile(strRecvBuffer,TcpServer.m_connfd,argv[2])==true)
  {
    strcpy(strSendBuffer,"ok");
    printf("ok.\n");
  }
  else
  {
    strcpy(strSendBuffer,"failed");
    printf("failed.\n");
  }

  // 向客户端返回响应内容
  if (TcpServer.Write(strSendBuffer)==false) 
  {
    printf("TcpServer.Write() failed.\n"); return -1;
  }

  printf("send:%s\n",strSendBuffer);

  return 0;
}

// 接收文件的内容
bool RecvFile(char *strRecvBuffer,int sockfd,char *strfilename)
{
  int  ufilesize=0;
  char strmtime[20]; 

  memset(strmtime,0,sizeof(strmtime));

  // 获取待接收的文件的时间和大小
  GetXMLBuffer(strRecvBuffer,"mtime",strmtime);
  GetXMLBuffer(strRecvBuffer,"size",&ufilesize);

  FILE *fp=NULL;

  if ( (fp=fopen(strfilename,"wb")) ==NULL)
  {
    printf("create %s failed.\n",strfilename); return false;
  }

  int  total_bytes=0;
  int  onread=0;
  char buffer[1000];

  while (true)
  {
    memset(buffer,0,sizeof(buffer));

    if ((ufilesize-total_bytes) > 1000) onread=1000;
    else onread=ufilesize-total_bytes;

    if (Readn(sockfd,buffer,onread) == false)
    {
      printf("Readn() failed.\n"); fclose(fp); fp=NULL; return false;
    }

    fwrite(buffer,1,onread,fp);

    total_bytes = total_bytes + onread;

    if ((int)total_bytes == ufilesize) break;
  }

  fclose(fp);

  // 重置文件的时间
  UTime(strfilename,strmtime);

  return true;
}

