#include <fstream>
#include <string>
#include <iostream>
#include <sstream>
#include <vector>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

void parseEntry(int offset, int maxoffset, std::string workdir);

struct dirent *drnt;
std::ifstream file;

int main(int argc, char *argv[])
{
    std::string fileName;
    if(argc>1)
        fileName = argv[1];
    else
    {
        std::cout<<"Podaj nazwę pliku"<<std::endl;
        return 0;
    }

    std::cout<<"Odczytywanie pliku .REZ ..."<<std::endl;
    
    file.open(fileName.c_str(), std::ios::in | std::ios::binary);
    if(!file)
    {
        std::cout<<"Nie znaleziono pliku!"<<std::endl;
        return 0;
    }

    //HEADER
    char *description; //LITHTECH
    description = new char [127];
    file.read(description, 127);

    int version; //VERSION
    file.read((char*)&version, 4);

    int offset; //DIR OFFSET
    file.read((char*)&offset, 4);

    int dsize; //DIR SIZE
    file.read((char*)&dsize, 4);

    int empty; //SOMETHING (?)
    file.read((char*)&empty, 4);

    int idxoffset; //IDXOFFSET
    file.read((char*)&idxoffset, 4);

    int datetime; //DATETIME
    file.read((char*)&datetime, 4);

    int empty2; //EMPTY2
    file.read((char*)&empty2, 4);

    int maxdirnamel; //MAXDIRNAMELENGTH
    file.read((char*)&maxdirnamel, 4);

    int maxfilenamel; //MAXFILENAMELENGTH
    file.read((char*)&maxfilenamel, 4);

    parseEntry(offset, offset + dsize, "");


    return 1;
}


std::string parseText(int maxoffset)
{
    std::string text="";
    int got=0;
    while(file.tellg()<=maxoffset)
    {
        file.read((char*)&got, 1);
        if(got==0)
            break;
        else
        {
            text += static_cast<char>(got);
        }
    }

    return text;
}


void deploy(int offset, int size, std::string fullPath)
{
    std::cout<<fullPath<<std::endl;

    if(size==0)
        return;

    std::string baseDir="files/";

    fullPath=baseDir+fullPath;

    std::vector<std::string> strippedPath;
    std::string fileName;
    
    std::string tmp;
    for(int i=0; i<fullPath.length(); i++)
    {
        if(fullPath.at(i)=='/')
        {
            strippedPath.push_back(tmp);
            tmp="";
        }
        else if(i==(fullPath.length()-1))
            fileName=tmp+fullPath.at(i);
        else
            tmp+=fullPath.at(i);
    }

    
    for(int i=0; i<strippedPath.size(); i++)
    {
        mkdir(strippedPath.at(i).c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        chdir(strippedPath.at(i).c_str());
    }

    char buffer[size];
    int lastoffset = file.tellg();
    file.seekg(offset);
    file.read(buffer, size);
    file.seekg(lastoffset);  
        
    std::ofstream writeFile;
    writeFile.open(fileName.c_str(), std::ios::out | std::ios::binary);
    writeFile.write (buffer, size);

    for(int i=0; i<strippedPath.size(); i++)
    {
        chdir("..");
    }
}

void parseEntry(int offset, int maxoffset, std::string workdir)
{
    file.seekg(offset);
    if(file.eof())
        return;

    //REZ_ENTRY DESCRIPTORS
    int entryType;
    file.read((char*)&entryType, 4);

    int entryOffset;
    file.read((char*)&entryOffset, 4);

    int entrySize;
    file.read((char*)&entrySize, 4);

    if(entrySize==0)
        return;

    int entryDateTime;
    file.read((char*)&entryDateTime, 4);   

    if(entryType == 0) //It's a file
    {
        int fileID;
        file.read((char*)&fileID, 4);
        
        std::string fileExtension=parseText(maxoffset);
        int null;
        file.read((char*)&null, 4); //Blank *4
        std::string fileName=parseText(maxoffset);

        std::string fullFile = workdir+fileName+"."+fileExtension;

        if(fileName.length()==0)
            return;

        deploy(entryOffset, entrySize, fullFile);

        file.read((char*)&null, 1); //Blank *1
        parseEntry((int)file.tellg(), (int)(file.tellg())+entrySize, workdir); 
    }
    else if(entryType == 1) //It's a dir
    {
        std::string dirName=parseText(maxoffset);

        if(dirName.length()==0)
            return;

        int oldOffset = file.tellg();

        if(entrySize>0)
            parseEntry(entryOffset, entryOffset+entrySize, workdir+dirName+"/"); //Recurrency of parsing directories

        parseEntry(oldOffset, oldOffset+entrySize, workdir); 
    }

}

