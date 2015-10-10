#include <stdio.h>
#include <unistd.h>

#include "pppd.h"
#include "chap-ty.h"
#include "chap-sync.h"
#include "ty_request.h"

char * requests =
	"sleep 5\n"
	"dial_account=\"%s\"\n"
	"update_time=$(date +%%Y-%%m-%%d+%%H:%%M:%%S)\n"
	"client_id=$(cat /proc/sys/kernel/random/uuid)\n"

	//request 1
	"url=\"http://client_int.yiqilai.cn:8080/esurfingclient/GetClientUserShareConfig.ashx?dialaccount=$dial_account\"\n"
	"(wget --user-agent \"MFC HTTPS Tear Fragment\" --header 'Cache-Control: no-cache' -q -O - \"$url\" 1>/dev/null 2>/dev/null) & pid=$!\n"
	"(sleep 2 && kill -9 $pid 2>/dev/null) &\n"

	//request 2
	"url=\"http://client_int.yiqilai.cn:8080/esurfingclient/GetInterface.aspx?KEY=JS_0002&DialUpAccount=$dial_account&DialUpType=3&Version=22\"\n"
	"(wget --user-agent \"EClientComm\" --header 'Cache-Control: no-cache' -q -O - \"$url\" 1>/dev/null 2>/dev/null) & pid=$!\n"
	"(sleep 2 && kill -9 $pid 2>/dev/null) &\n"

	//request 3
	"url=\"http://client_int.yiqilai.cn:8080/esurfingclient/Banners/banner33.html\"\n"
	"(wget --user-agent \"Watcher\" --header 'Cache-Control: no-cache' -q -O - \"$url\" 1>/dev/null 2>/dev/null) & pid=$!\n"
	"(sleep 2 && kill -9 $pid 2>/dev/null) &\n"

	//request 4
	"url=\"http://client_int.yiqilai.cn:8080/esurfingclient/GetMarketingData.ashx?Account=$dial_account&Token=&UpdateTime=$update_time&IsFirstTime=1&Versions=21&Language=2052_936&DialUpnum=&DialUptype=3&ATI=&IPv6=&UpLoad=0&DownLoad=0&ClientID=$client_id\"\n"
	"(wget --user-agent \"EClientComm\" --header 'Cache-Control: no-cache' -q -O - \"$url\" 1>/dev/null 2>/dev/null) & pid=$!\n"
	"(sleep 2 && kill -9 $pid 2>/dev/null) &\n"

	//request 5
	"url=\"http://client_int.yiqilai.cn:8080/esurfingclient/GetClientUserShareConfig.ashx?dialaccount=$dial_account\"\n"
	"(wget --user-agent \"MFC HTTPS Tear Fragment\" --header 'Cache-Control: no-cache' -q -O - \"$url\" 1>/dev/null 2>/dev/null) & pid=$!\n"
	"(sleep 2 && kill -9 $pid 2>/dev/null) &\n"

	//request 6
	"url=\"http://interface.tykd.vnet.cn/QueryCityClient/QueryUserCity.ashx?BusCode=ProxyPlug1&Version=22&Language=2052_936&DialupType=3&DialupName=$dial_account&Passport=\"\n"
	"(wget --user-agent \"_HttpClient\" --header 'Cache-Control: no-cache' -q -O - \"$url\" 1>/dev/null 2>/dev/null) & pid=$!\n"
	"(sleep 2 && kill -9 $pid 2>/dev/null) &\n"

	//request 7
	"url=\"http://interface.tykd.vnet.cn/QueryCityClient/QueryUserCity.ashx?BusCode=ProxyPlug2&Version=22&Language=2052_936&DialupType=3&DialupName=$dial_account&Passport=\"\n"
	"(wget --user-agent \"_HttpClient\" --header 'Cache-Control: no-cache' -q -O - \"$url\" 1>/dev/null 2>/dev/null) & pid=$!\n"
	"(sleep 2 && kill -9 $pid 2>/dev/null) &\n"

	//request 8
	"url=\"http://client_int.yiqilai.cn:8080/esurfingclient/GetClientUserShareConfig.ashx?dialaccount=$dial_account\"\n"
	"(wget --user-agent \"MFC HTTPS Tear Fragment\" --header 'Cache-Control: no-cache' -q -O - \"$url\" 1>/dev/null 2>/dev/null) & pid=$!\n"
	"(sleep 2 && kill -9 $pid 2>/dev/null) &\n"

	//request 9
	"url=\"http://client_int.yiqilai.cn:8080/esurfingclient/GetMarketingData.ashx?Account=$dial_account&Token=&UpdateTime=$update_time&IsFirstTime=1&Versions=21&Language=2052_936&DialUpnum=&DialUptype=3&ATI=&IPv6=&UpLoad=0&DownLoad=0&ClientID=$client_id&IsSupportUw=0&WifiMac=\"\n"
	"(wget --user-agent \"EClientComm\" --header 'Cache-Control: no-cache' -q -O - \"$url\" 1>/dev/null 2>/dev/null) & pid=$!\n"
	"(sleep 2 && kill -9 $pid 2>/dev/null) &\n"

	//request 10
	"url=\"http://client_int.yiqilai.cn:8080/esurfingclient/GetClientUserShareConfig.ashx?dialaccount=$dial_account\"\n"
	"(wget --user-agent \"MFC HTTPS Tear Fragment\" --header 'Cache-Control: no-cache' -q -O - \"$url\" 1>/dev/null 2>/dev/null) & pid=$!\n"
	"(sleep 2 && kill -9 $pid 2>/dev/null) &\n"
	"sleep 3\n"
	"exit\n";

/*
* 自己实现的popen，打开一个bash，返回写入的管道，并且不记录子进程pid，在这里假设子进程会自行退出
*/
FILE *shpopen()
{
	int pipefd[2]; // 管道描述符
	int pid_t; // 进程描述符
	
	if(pipe(pipefd) < 0){
		return NULL;
	}
	
	pid_t = fork();
	
	if(pid_t < 0){
		return NULL;
	}
	
	if(0 == pid_t){ // child
		close(pipefd[1]);  
		if (pipefd[0] != STDIN_FILENO) {  
			dup2(pipefd[0], STDIN_FILENO);  
			close(pipefd[0]);  
		}
		char * argv[] ={"sh", 0};
		execvp("sh", argv);
		
		_exit(127);
	}
	
	close(pipefd[0]);  
	return fdopen(pipefd[1],"w"); 
}

void doTyRequestR(){
	char req[5000] = {0};
	FILE* shFp = shpopen();
	if(shFp == NULL){
		return;
	}
	
	slprintf(req, sizeof(req), requests, user);
	
	fputs(req, shFp);
	fclose(shFp);
}

void doTyRequest(){
	if(!ty_dial || chap_sync_count != 1) // 只有在不启用多拨时才执行http请求
		return;
		
	doTyRequestR();
}
