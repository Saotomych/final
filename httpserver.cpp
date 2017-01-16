#include <unistd.h>
#include <string.h>
#include <time.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <thread>
#include <string>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <map>
#include <algorithm>
#include <memory>
#include <boost/algorithm/string.hpp>

using namespace std;

string ipaddr;
uint16_t port;
string homedir;

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

void getoptions(int argc, char** argv)
{
	int res;

	while ( (res = getopt(argc,argv,"h:p:d:")) != -1){
			switch (res){
			case 'h': 
				printf("found argument \"h = %s\"\n",optarg);
				ipaddr = optarg;
				break;
			case 'p': 
				printf("found argument \"p = %s\"\n",optarg);
				port = atoi(optarg);
				break;
			case 'd': 
				printf("found argument \"d = %s\"\n",optarg);
				homedir = optarg;
				break;
			case '?': printf("Error found !\n");break;
	        };
	}
}

bool GetProcessing(stringstream& res, string fn)
{
/*	
	HTTP/1.0 200 OK 
	Date: Fri, 08 Aug 2003 08:12:31 GMT 
	Server: Apache/1.3.27 (Unix) 
	MIME-version: 1.0 
	Last-Modified: Fri, 01 Aug 2003 12:45:26 GMT 
	Content-Type: text/html 
	Content-Length: 2345 
	** a blank line * 
	<HTML> ...
*/	
	
	cout << "Response for file: " << fn << endl;
	
	bool ret;

	// Read file to stringstream buffer
	stringstream strStream;
	size_t filesize = 0;

	unique_ptr<ifstream> hFl(new ifstream(fn));
	if (!hFl->is_open())
	{
		res << "HTTP/1.0 404 Page not found" << endl;
		ret = false;
	}
	else
	{
		res << "HTTP/1.0 200 OK" << endl;
		ret = true;
   
		copy( (istreambuf_iterator<char>(*(hFl).get() )),
			istreambuf_iterator<char>(),
			ostreambuf_iterator<char>(strStream)
			);
		
		filesize = hFl->tellg();
		hFl->close();
	}

	res << "Date: Fri, 08 Aug 2003 08:12:31 GMT" << endl; 
	res << "Server: Custom (Unix)" << endl; 
	res << "MIME-version: 1.0" << endl; 
	res << "Last-Modified: Fri, 01 Aug 2003 12:45:26 GMT" << endl; 
	res << "Content-Type: text/html" << endl; 
	res << "Connection: close" << endl;
	
	if (ret)
	{ 
		res << "Content-Length: " << filesize << endl << endl;
		res << strStream.str() << endl;
	}
	else
	{
		string resp404("404 Page not found");
		res << "Content-Length: " << resp404.size() << endl << endl;
		res << resp404 << endl;
	}
	
	return ret;
}

void clientworker(int clientsd)
{
	cout << "Client thread started for client: " << clientsd << endl;
	
	char buffer[256];
	
	int n = read(clientsd, buffer, sizeof(buffer)-1);
	if (n < 0) 
		error("ERROR reading from socket");

/*	
	GET /index.html HTTP/1.1
	User-Agent: curl/7.35.0
	Host: 127.0.0.1:12345
	Accept: *//*
*/

	printf("Here is the message: %s\n",buffer);
	
//	std::map<std::string, std::string> headers;
//	std::istringstream rawstream(buffer);
//	std::string header;
//	while (std::getline(rawstream, header) && header != "\r") {
//		std::string::size_type index = header.find(':');
//		if (index != std::string::npos) {
//			headers.insert(std::make_pair(
//				boost::algorithm::to_lower_copy(boost::algorithm::trim_copy(header.substr(0, index))),
//				boost::algorithm::trim_copy(header.substr(index + 1))
//			));
//		}
//	}
// 
// 	for (auto& kv : headers) {
// 		std::cout << kv.first << ": " << kv.second << std::endl;
// 	}
	
	stringstream response;
	string request(buffer, 4);
	if ( request == string("GET "))
	{
		char* p = find(&buffer[4], &buffer[255], ' ');
		string filename(&buffer[4], p-(&buffer[4]));
		if (filename.size() <= 2)
		{
			cout << "Filename is not explicit. " << endl;
			if ( GetProcessing(response, string(homedir+filename+string("index.htm"))) == false )
			{
				stringstream s;
				swap(s, response);
				if ( GetProcessing(response, string(homedir+filename+string("index.html"))) == false )
				{
					stringstream s;
					swap(s, response);
					if ( GetProcessing(response, string(homedir+filename+string("index.php"))) == false )
					{
						cout << "Default files not found" << endl;
					}
				}
			}
		}
		else
		{
			cout << "Filename is explicit: " << homedir << filename << endl;
			GetProcessing(response, string(homedir+filename));
		}
	}

	cout << response.str() << endl;
	
	n = write(clientsd, response.str().c_str(), response.str().size());
	if (n < 0) 
		error("ERROR writing to socket");
		
	shutdown(clientsd, SHUT_RDWR);
	close(clientsd);

	cout << "Client thread end for client: " << clientsd << endl;
}

int main(int argc, char** argv)
{
	getoptions(argc, argv);
	
	int serversd  = socket(AF_INET, SOCK_STREAM, 0);
    if (serversd < 0) 
       error("ERROR opening socket");
    
    struct sockaddr_in serv_addr;
    bzero((char *) &serv_addr, sizeof(serv_addr));
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);

    if (bind(serversd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
             error("ERROR on binding");
    
    for(;;)
    {
        listen(serversd, SOMAXCONN);
        
        struct sockaddr_in client_addr;
        socklen_t sz = sizeof(client_addr);
        int clientsd = accept(serversd, (struct sockaddr *) &client_addr, &sz);
        if (clientsd < 0) 
        	error("ERROR on accept");
            
        thread thr(clientworker, clientsd);
        thr.detach();    	
    }
}

