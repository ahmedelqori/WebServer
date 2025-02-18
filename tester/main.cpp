#include "./googletest/googletest/include/gtest/gtest.h"
#include <unistd.h> 
#include <sys/wait.h>  
#include <dirent.h>  
#include <vector>
#include <string>
#include <iostream>
#include <unistd.h>
#include <fcntl.h>

bool executeWebservWithFile(const std::string& filePath) {
    pid_t pid = fork();
    if (pid == -1) {
        std::cerr << "Fork failed!" << std::endl;
        return false;
    } else if (pid == 0) {
        int fd = open("./logs/invalid.log",O_RDWR | O_CREAT | O_APPEND, 0777);
        dup2(fd, STDERR_FILENO);
        std::string program = "../webserv";
        std::vector<char*> args = {
            const_cast<char*>(program.c_str()),
            const_cast<char*>(filePath.c_str()),
            nullptr
        };
        execve(program.c_str(), args.data(), environ);
        std::cerr << "execve failed for file: " << filePath << std::endl;
        _exit(EXIT_FAILURE);
    } else {
        int status;
        waitpid(pid, &status, 0);

        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            return true; 
        } else {
            return false; 
        }
    }
}

TEST(WebservTest, InvalidFilesTest) {
    const std::string directory = "./conf/invalid";
    remove("./logs/invalid.log");
    DIR* dir = opendir(directory.c_str());
    if (!dir) {
        FAIL() << "Failed to open directory: " << directory;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string fileName = entry->d_name;
        if (fileName == "." || fileName == "..") {
            continue; 
        }

        std::string filePath = directory + "/" + fileName;

        bool success = executeWebservWithFile(filePath);

        EXPECT_FALSE(success) << "Expected failure for file: " << filePath;
    }

    closedir(dir);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}