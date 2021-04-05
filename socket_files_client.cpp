//
// ��������ʾ����CTcpClient�࣬ʵ��socketͨѶ�Ŀͻ��˺��ļ����䡣
// 

#include "_freecplus.h"

// ���ļ������ݷ��͸������
bool SendFile(int sockfd,char *filename,int filesize);

int main(int argc,char *argv[])
{
  if (argc != 4)
  {
    printf("\n");
    printf("Using:./demo13 ip port filename\n\n");

    printf("Example:./demo13 118.89.50.198 5010 test1.jpg\n\n");

    printf("��������ʾ����CTcpClient�࣬ʵ��socketͨѶ�Ŀͻ��˺��ļ����䡣\n\n");

    return -1;
  }

  // �ж��ļ��Ƿ��
  if (access(argv[3],R_OK) != 0)
  {
    printf("file %s not exist.\n",argv[3]); return -1;
  }

  int uFileSize=0;
  char strMTime[20],strRecvBuffer[1024],strSendBuffer[1024];

  // ��ȡ�ļ���ʱ��ʹ�С
  memset(strMTime,0,sizeof(strMTime));
  FileMTime(argv[3],strMTime);

  // ��ȡ�ļ��Ĵ�С
  uFileSize=FileSize(argv[3]);

  // ���ļ�����Ϣ��װ��һ��xml�����͸������
  memset(strSendBuffer,0,sizeof(strSendBuffer));
  snprintf(strSendBuffer,100,"<filename>%s</filename><mtime>%s</mtime><size>%lu</size>",argv[3],strMTime,uFileSize);

  CTcpClient TcpClient;

  // ���������������
  if (TcpClient.ConnectToServer(argv[1],atoi(argv[2])) == false)
  {
    printf("TcpClient.ConnectToServer(%s,%d) failed.\n",argv[1],atoi(argv[2])); return -1;
  }

  // ���ļ���Ϣ��xml���͸������
  if (TcpClient.Write(strSendBuffer)==false)
  {
    printf("TcpClient.Write() failed.\n"); return -1;
  }

  printf("send xml:%s\n",strSendBuffer);

  printf("send file ...");

  // ���ļ������ݷ��͸������
  if (SendFile(TcpClient.m_sockfd,argv[3],uFileSize)==false)
  {
    printf("SendFile(%s) failed.\n",argv[3]); return -1;
  }

  memset(strRecvBuffer,0,sizeof(strRecvBuffer));

  // ���շ���˷��ص�ȷ�ϱ���
  if (TcpClient.Read(strRecvBuffer)==false)
  {
    printf("TcpClient.Read() failed.\n"); return -1;
  }

  if (strcmp(strRecvBuffer,"ok")==0)
    printf("ok.\n");
  else
    printf("failed.\n");

  return 0;
}

// ���ļ������ݷ��͸������
bool SendFile(int sockfd,char *filename,int filesize)
{
  int  bytes=0;
  int  total_bytes=0;
  int  onread=0;
  char buffer[1000];

  FILE *fp=NULL;

  if ( (fp=fopen(filename,"rb")) == NULL ) 
  {
    printf("fopen(%s) failed.\n",filename); return false;
  }

  while (true)
  {
    memset(buffer,0,sizeof(buffer));

    if ((filesize-total_bytes) > 1000) onread=1000;
    else onread=filesize-total_bytes;

    bytes=fread(buffer,1,onread,fp);

    if (bytes > 0)
    {
      if (Writen(sockfd,buffer,bytes) == false)
      {
        printf("Writen() failed.\n"); fclose(fp); fp=NULL; return false;
      }
    }

    total_bytes = total_bytes + bytes;

    if ((int)total_bytes == filesize) break;
  }

  fclose(fp);

  return true;
}

