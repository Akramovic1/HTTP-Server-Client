#include <iostream>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <pthread.h>
#include <string>
#include <bits/stdc++.h>

using namespace std;

string OK_RESPOND = "HTTP/1.1 200 OK\\r\\n \n";
string NOT_FOUND_RESPOND = "HTTP/1.1 404 Not Found\\r\\n \n";
void * clientConnection(void* P_socketClient);
void * clientTimeOut (void *);
void sendChunks(int socket , string s);
string readFileContent(string photoName);
string getContentType(string fname);
string getRequestedFileContent(string fileName);
void saveFile (string fileName, string content);
bool checkFileExist(string fileName);
string trim(const string& str);
vector<string> splitRequest(string str);
struct arg_struct {int client_socket;long long* timer;};
vector <arg_struct*> clientsStructs;

int main(int argc, char const *argv[]){
    if (argc != 2){printf("Not Valid Arguments or Missing Port");exit(EXIT_FAILURE);}
    int server_address, new_socket;
    server_address = socket(AF_INET, SOCK_STREAM, 0);
    int portNum;
    struct sockaddr_in address;
    portNum = atoi(argv[1]);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(portNum);
    memset(address.sin_zero, '\0', sizeof address.sin_zero);
    int address_len = sizeof(address);
    if (server_address < 0){printf("Error Creating Server Socket");exit(EXIT_FAILURE);}
    int bind_status = bind(server_address, (struct sockaddr *)&address, sizeof(address));
    if (bind_status < 0){printf("Error Binding The Server");exit(EXIT_FAILURE);}
    int listen_status = listen(server_address, 10) ;
    if (listen_status < 0){printf("Error In Listening");exit(EXIT_FAILURE);}
    pthread_t timeout;
    pthread_create(&timeout, NULL, clientTimeOut, NULL);
    while(1){
        cout << "Waiting for a connection" << endl;
        cout << flush;
        if ((new_socket = accept(server_address, (struct sockaddr *)&address, (socklen_t*)&address_len)) < 0){
            printf("Error Accepting Connection");
            exit(EXIT_FAILURE);
        }
        pthread_t t;
        long long myClock = 0;
        arg_struct argues;
        argues.timer = &myClock;
        argues.client_socket = new_socket;
        clientsStructs.push_back(&argues);
        pthread_create(&t, NULL, clientConnection, &argues);
    }
    return 0;
}

void * clientTimeOut (void *){
    while (1){
        for (int i = 0; i < clientsStructs.size() ; i++){
            long long interval = clock() - *(clientsStructs[i]->timer);
            if (interval >= 1e4){
                close((clientsStructs[i]->client_socket));
                clientsStructs.erase(clientsStructs.begin()+i);
                i--;
                cout << "A Client Connection Is Closed" << endl;
            }
        }
        sleep(1);
    }
}

void * clientConnection(void* P_socketClient)
{
    string total_buffer = "";
    arg_struct clientStruct = *((arg_struct*) P_socketClient);
    int socketClient = clientStruct.client_socket;
    long long* currentClock = clientStruct.timer;
    *(currentClock) = clock();
    while (1){
        *(currentClock) = clock();
        char buffer[4096] = {0};
        long long val = read(socketClient, buffer, 4096);
        if (val <= 0) break;
        cout << buffer << endl;
        cout << flush;
        buffer[strlen(buffer) - 1] = '\0';
        total_buffer += buffer;
        vector<string> splits = splitRequest(total_buffer);
        if (splits[0] == "get" || splits[0] == "client_get") {
            string fileName = splits[1];
            if (checkFileExist(fileName)){
                string ok = OK_RESPOND;
                char * tab2 = &ok[0];
                write(socketClient, tab2, strlen(tab2));
                string content = readFileContent(fileName);
                int fileSize = content.size();
                write(socketClient, &fileSize, sizeof(int));
                sendChunks(socketClient, content);
            } else {
                string str = NOT_FOUND_RESPOND;
                char * tab2 = &str[0];
                write(socketClient, tab2, strlen(tab2));
                string content = readFileContent("not_found.txt");
                int fileSize = content.size();
                write(socketClient, &fileSize, sizeof(int));
                sendChunks(socketClient, content);
            }

        } else if (splits[0] == "post" || splits[0] == "client_post"){
            string fileName = splits[1];
            string str = OK_RESPOND;
            char * tab2 = &str[0];
            write(socketClient, tab2, strlen(tab2));
            bzero(buffer, 4096);
            long long val2 = read(socketClient, buffer, 4096);
            if (val2 <= 0) break;
            cout << buffer << endl;
            cout << flush;
            string fileToSave = string(buffer);
            saveFile(fileName, fileToSave);
            string res = "File is saved successfully \n";
            char * msg = &res[0];
            write(socketClient, msg, strlen(msg));
        } else if (splits[0] == "GET") {
            string fileName = splits[1];
            string real = "";
            for (int i = 1; i < fileName.size() ; i++){real += fileName[i];}
            if (checkFileExist(real)){
                cout << endl << real << endl;
                string fContent =  getRequestedFileContent(real);
                int cLength = fContent.size();
                string conType = getContentType(real);
                string str = "HTTP/1.1 200 OK\nContent-Type: " + conType+ "\nContent-Length: " + to_string(cLength) + "\n\n" + getRequestedFileContent(real);
                char * tab2 = &str[0];
                write(socketClient, tab2, strlen(tab2));
            } else {
                string fContent =  getRequestedFileContent("not_found.txt");
                int cLength = fContent.size();
                string conType = getContentType(fileName);
                string str = "HTTP/1.1 404 Not Found\n";
                char * tab2 = &str[0];
                write(socketClient, tab2, strlen(tab2));
            }
        } else if (splits[0] == "close") {
            break;
        } else {
            string str = NOT_FOUND_RESPOND;
            char * tab2 = new char [str.length()+1];
            strcpy (tab2, str.c_str());
            write(socketClient, tab2, strlen(tab2));
        }
        bzero(buffer, 4096);
        total_buffer = "";
    }
    close(socketClient);
    return NULL;
}

string getContentType(string fname){
    if (fname.find(".txt") != std::string::npos)
        return "text/plain";
    if (fname.find(".html") != std::string::npos)
        return "text/html";
}

vector<string> splitRequest(string str){
    str = trim(str);
    vector<string> vec;
    int counter = 0;
    string word = "";
    for (auto x : str){
        if (x == ' '){
            vec.push_back(word);
            word = "";
            counter++;
            if (counter == 2) break;
        }
        else
            word = word + x;
    }
    return vec;
}


string readFileContent(string fileName){
    ifstream fin(fileName);
    size_t buffer_size = 1024;
    char buffer[buffer_size];
    size_t len = 0;
    ostringstream streamer;
    while((len = fin.readsome(buffer, buffer_size)) > 0)
        streamer.write(buffer, len);
    return streamer.str();
}

void sendChunks(int socket , string s) {
    const char *beginner = s.c_str();
    for (int i = 0; i < s.length(); i += 500)
        send(socket, beginner + i,min(500, (int)s.length() - i), 0);
}


string getRequestedFileContent(string fileName){
    string line;
    string content = "";
    ifstream myfile;
    myfile.open(fileName);
    while(getline(myfile, line))
        content += line;
    return content;
}

void saveFile (string fileName, string content){
    ofstream f_stream(fileName.c_str());
    f_stream.write(content.c_str(), content.length());
}

bool checkFileExist(string fileName){
    ifstream ifile;
    ifile.open(fileName);
    if(ifile)
        return true;
    else
        return false;
}

string trim(const string& str){
    size_t first = str.find_first_not_of(' ');
    if (string::npos == first)
        return str;
    size_t last = str.find_last_not_of(' ');
    return str.substr(first, (last - first + 1));
}

