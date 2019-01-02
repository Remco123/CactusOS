#include <dirent.h>
#include <vector>
#include <cstring>
#include <stdint.h>
#include <stddef.h>  
#include <string>
#include <stdio.h>
#include <fstream>
#include <sys/stat.h>

using namespace std;

struct InitrdFileHeader
{
    char name[30];
    char path[100];
    uint32_t size;
} __attribute__((packed));  

long GetFileSize(std::string filename)
{
    struct stat stat_buf;
    int rc = stat(filename.c_str(), &stat_buf);
    return rc == 0 ? stat_buf.st_size : -1;
}

void GetReqDirs(const string& path, vector<string>& files, const bool showHiddenDirs = false)
{
    DIR *dpdf;
    struct dirent *epdf;
    dpdf = opendir(path.c_str());
    if (dpdf != NULL){
        while ((epdf = readdir(dpdf)) != NULL){
            if(showHiddenDirs ? (epdf->d_type==DT_DIR && string(epdf->d_name) != ".." && string(epdf->d_name) != "." ) : (epdf->d_type==DT_DIR && strstr(epdf->d_name,"..") == NULL && strstr(epdf->d_name,".") == NULL ) ){
                GetReqDirs(path+epdf->d_name+"/",files, showHiddenDirs);
            }
            if(epdf->d_type==DT_REG){
                files.push_back(path+epdf->d_name);
            }
        }
    }
    closedir(dpdf);
}
int main()
{
    char* path = "../../initrd/";
    char* targetPath = "../../isofiles/initrd";

    remove(targetPath);

    vector<string> allFiles;
    GetReqDirs(path, allFiles);

    for(int i = 0; i < allFiles.size(); i++)
    {
        const char* fileName = allFiles[i].c_str();
        printf("Adding: "); printf(fileName + strlen(path) - 1); printf("\n");
        
        std::ofstream fileBuffer;
        fileBuffer.open(targetPath, std::ios_base::app);

        //Write Header
        InitrdFileHeader header;
        memset(&header, 0, sizeof(InitrdFileHeader));
        memcpy(header.name, (string(fileName).substr(string(fileName).find_last_of("/\\") + 1).c_str()), (string(fileName).substr(string(fileName).find_last_of("/\\") + 1).size()));
        memcpy(header.path, fileName + strlen(path) - 1, strlen(fileName + strlen(path) - 1));
        header.size = GetFileSize(string(fileName));

        fileBuffer.write((const char*)&header, sizeof(InitrdFileHeader));
        
        //Write file data
        std::ifstream fileInBuffer; fileInBuffer.open(fileName, std::ios_base::app);

        auto buf = new char[header.size];
        fileInBuffer.read(buf, header.size);

        fileBuffer.write(buf, header.size);

        delete buf;
    }
}