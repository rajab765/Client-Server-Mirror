//***Advanced Systems Programming--Project***

// below 2 enables extensions in glibc library
#define _XOPEN_SOURCE
#define _GNU_SOURCE

//***Below are the appropriate header files used in the program***
//***The below line of code includes headerFile for stndrd Inp/Op library  
#include <stdio.h>
//***The below line of code includes headerFile for stndrd library for different functions like m/m  
#include <stdlib.h>
#include <time.h> 
//***The below line of code includes headerFile for string manipulation, m/m copying, string cmp
#include <string.h>
//***The below line of code includes headerFile for accessing multiple stndrd POSIX funcs
#include <unistd.h>
//***The below line of code includes headerFile for handling internet addresses and IP addresses
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <errno.h>
#include <limits.h>
#include <dirent.h>
#include <libgen.h>
//***The below line of code includes headerFile for implementing process controls and waiting for chld prcess
#include <sys/wait.h>
#include <stdbool.h>
#include <fnmatch.h>
 

#define RPPMCRO_cncPort4Server 6312
#define RPPMCRO_maxPermClients 20
#define RPPMCRO_ioBufferLength 2048
#define RPPMCRO_cncPort4Mirror 6313

bool var4ErrAck = false;

// Now what this func does is it sends temp tarball file to client. So now it checks if fil present, open & sends siz to client.
//  Furthermore, forks child process to transmit the contents of the file to the client. The parent process holds off on
//  closing the file until the child process finishes. If file is missing, the client gets a "TEMP_TAR_NOT_FOUND" notification.
void func4TransferTempTarFile(int param4clntSckt) {
    struct stat var4FileStat;
    if (stat("temp.tar.gz", &var4FileStat) == 0) {
        FILE *var4FilePtr = fopen("temp.tar.gz", "rb");
        if (var4FilePtr == NULL) {
            perror("***Failing of open Operation of File***");
            return;
        }

        fseek(var4FilePtr, 0, SEEK_END);
        size_t var4LengthOfFile = ftell(var4FilePtr);
        fseek(var4FilePtr, 0, SEEK_SET);

        send(param4clntSckt, "TEMP_TAR_READY", 15, 0);
        send(param4clntSckt, &var4LengthOfFile, sizeof(size_t), 0);

        char varBfr[RPPMCRO_ioBufferLength];
        size_t var4LeftBytes = var4LengthOfFile;

        // Forkin a child pr0cess
        pid_t var4ChldPrcID = fork();

        if (var4ChldPrcID == -1) {
            perror("***Failing of Fork Operation***");
            fclose(var4FilePtr);
            return;
        }

        if (var4ChldPrcID == 0) {  // Child pr0cess
            close(STDOUT_FILENO);  // Close stdout output
            dup2(param4clntSckt, STDOUT_FILENO);  // Redir stdout to client's socket

            while (var4LeftBytes > 0) {
                size_t var4TotalBytes2Tranfer = (var4LeftBytes < sizeof(varBfr)) ? var4LeftBytes : sizeof(varBfr);
                size_t var4TotalBytes2Read = fread(varBfr, 1, var4TotalBytes2Tranfer, var4FilePtr);
                if (var4TotalBytes2Read == 0) {
                    break;
                }

                write(STDOUT_FILENO, varBfr, var4TotalBytes2Read);
                var4LeftBytes -= var4TotalBytes2Read;
            }

            fclose(var4FilePtr);
            exit(EXIT_SUCCESS);
        } else {  // Parent pr0cces
            int varSttsFlag;
            waitpid(var4ChldPrcID, &varSttsFlag, 0);  // Wait for the child process to finish
            fclose(var4FilePtr);
        }
    } else {
        send(param4clntSckt, "TEMP_TAR_NOT_FOUND", 18, 0);
    }
}


//Now what the fgets command does is it gets llist from the client, tokenize it and copies file to temp dir. with the help of used ext.
//It then sees if file copied, makes tarball of temp dir and sends msg to client regardless of successs or errorr and removes temp dir.
void func4FgetsCmndExecution(int param4ClntSckt, const char *param4Cmnd) {
    char var4FileArray[RPPMCRO_ioBufferLength];
    sscanf(param4Cmnd, "fgets %[^\n]", var4FileArray);

    // Tokenise fil lest
    char *var4rToken = strtok(var4FileArray, " ");
    int var4TotalFls = 0;

    // Make a direc for temporary files.
    char var4TmprDrctry[] = "temp_files";
    mkdir(var4TmprDrctry, 0700);

    // the permitted file extensions list
    const char *var4CorrectExtnsnsArray[] = { "txt", "pdf", "c", "jpg", "png", "jpeg" };
    int var4TotalPermExtnsns = sizeof(var4CorrectExtnsnsArray) / sizeof(var4CorrectExtnsnsArray[0]);

    // Files with permitted extensions should be copied to the temporary directory.
    while (var4rToken != NULL && var4TotalFls < 4) {
        char var4Cmnd[RPPMCRO_ioBufferLength];
        char var4NameOfFile[RPPMCRO_ioBufferLength];
        char var4NameOfExtnsn[RPPMCRO_ioBufferLength];
        sscanf(var4rToken, "%[^.].%s", var4NameOfFile, var4NameOfExtnsn);

        // Check that the file extension is recognised.
        int var4PermissibleFlag = 0;
        for (int z = 0; z < var4TotalPermExtnsns; z++) {
            if (strcmp(var4NameOfExtnsn, var4CorrectExtnsnsArray[z]) == 0) {
                var4PermissibleFlag = 1;
                break;
            }
        }

        if (var4PermissibleFlag) {
            snprintf(var4Cmnd, sizeof(var4Cmnd), "find $HOME -name '%s' -type f -exec cp {} %s/ \\;", var4rToken, var4TmprDrctry);
            system(var4Cmnd);
            var4TotalFls++;
        } else {
            printf("Ignoring the file '%s'. Not Permissible Extension '%s'\n", var4NameOfFile, var4NameOfExtnsn);
        }

        var4rToken = strtok(NULL, " ");
    }

    // We'll check to see if any files were copied.
    int var4FilesCopied = 0;
    DIR *var4PtrTempDrctry = opendir(var4TmprDrctry);
    if (var4PtrTempDrctry) {
        struct dirent *var4EntryPtr;
        while ((var4EntryPtr = readdir(var4PtrTempDrctry)) != NULL) {
            if (var4EntryPtr->d_type == DT_REG) {
                var4FilesCopied = 1;
                break;
            }
        }
        closedir(var4PtrTempDrctry);
    }

    if (var4FilesCopied) {
        // Make a tarball of your temporary directory.
        char var4TarFlCmnd[RPPMCRO_ioBufferLength];
        snprintf(var4TarFlCmnd, sizeof(var4TarFlCmnd), "tar czf temp.tar.gz %s", var4TmprDrctry);

        // To run the tar command, fork a child process.
        pid_t var4ChldPrcID = fork();

        if (var4ChldPrcID == -1) {
            perror("***Failing of Fork Operation***");
            return;
        }

        if (var4ChldPrcID == 0) {  // Child pr0ccces
            int var4DevNullPath = open("/dev/null", O_WRONLY);
            dup2(var4DevNullPath, STDOUT_FILENO);  // Stdout has to be redirected to /dev/null.
            close(var4DevNullPath);

            execl("/bin/sh", "/bin/sh", "-c", var4TarFlCmnd, (char *)NULL);
            exit(EXIT_FAILURE);  // Eczit child on failure
        } else {  // Parent pr0cces
            int varSttsFlag;
            waitpid(var4ChldPrcID, &varSttsFlag, 0);  // Pause/Wai for child procses to complete
        }

        // Send the client a token of success.
        const char *var4AcknRespMsg = "Fiiles Arrchaived in the file temp.tar.gz";
        send(param4ClntSckt, var4AcknRespMsg, strlen(var4AcknRespMsg), 0);
    } else {
        // Send error ack. to client
        const char *var4AcknRespMsg = "No Fiiles Found in Drctry";
        send(param4ClntSckt, var4AcknRespMsg, strlen(var4AcknRespMsg), 0);
    }

    // Clears temp. direc.
    char var4RmvFlCmnd[RPPMCRO_ioBufferLength];
    snprintf(var4RmvFlCmnd, sizeof(var4RmvFlCmnd), "rm -r %s", var4TmprDrctry);

    // Create a child pr0cces and fork it to run rem command.
    pid_t var4ChldPrcID = fork();

    if (var4ChldPrcID == -1) {
        perror("***Failing of Fork Operation***");
        return;
    }

    if (var4ChldPrcID == 0) {  // Child pr0cces
        int var4DevNullPath = open("/dev/null", O_WRONLY);
        dup2(var4DevNullPath, STDOUT_FILENO);  // Redir stdout to /dev/nul
        close(var4DevNullPath);

        execl("/bin/sh", "/bin/sh", "-c", var4RmvFlCmnd, (char *)NULL);
        exit(EXIT_FAILURE);  // Exit child pr0cces if unsuccessful
    } else {  // Parent pr0cces
        int varSttsFlag;
        waitpid(var4ChldPrcID, &varSttsFlag, 0);  // Wait for the child process to finish
    }
}


//Now for this command, it parses cmnd for the size args., followed by makinng a temo dir. and forks a child pr0cces.
    //It also sends the msg of succcesss or erorr, if the size and ext criterea are been matched by the child and then it copies to temp dir
        //and then makes tarbal. atlast ofcourse removes temp dir asususal.

void func4TarfgetzCmndExecution(int param4ClntSckt, const char *param4Cmnd) {
    long long var4FirstSizeArg, var4SecondSizeArg;

    // Parse cmand args.
    int var4ParsedCmndRes = sscanf(param4Cmnd, "tarfgetz %lld %lld %*[-u]", &var4FirstSizeArg, &var4SecondSizeArg);

    // Make dir for temp files for storing.
    char var4TmprDrctry[] = "temp_files";
    mkdir(var4TmprDrctry, 0700);

    // Fork a child pr0cces
    pid_t var4ChldPrcID = fork();

    if (var4ChldPrcID == -1) {
        perror("***Failing of Fork Operation***");
        return;
    }

    if (var4ChldPrcID == 0) {  // Child pr0cces
        // Stdout has to be redirected to the client socket.
        dup2(param4ClntSckt, STDOUT_FILENO);

        // List of permited file extensin
        const char *var4CorrectExtnsnsArray[] = { "txt", "pdf", "c", "jpg", "png", "jpeg" };
        int var4TotalPermExtnsns = sizeof(var4CorrectExtnsnsArray) / sizeof(var4CorrectExtnsnsArray[0]);

        // Search fils that meets size&extensin condition/req.
        char var4Cmnd[RPPMCRO_ioBufferLength];
        snprintf(var4Cmnd, sizeof(var4Cmnd), "find $HOME -type f \\( \\( -size +%lldc -a -size -%lldc \\) -o \\( -size %lldc -o -size %lldc \\) \\) \\( -name '*.%s' -o -name '*.%s' -o -name '*.%s' -o -name '*.%s' -o -name '*.%s' -o -name '*.%s' \\) -exec cp {} %s/ \\;",
                 var4FirstSizeArg, var4SecondSizeArg, var4FirstSizeArg, var4SecondSizeArg,
                 var4CorrectExtnsnsArray[0], var4CorrectExtnsnsArray[1], var4CorrectExtnsnsArray[2],
                 var4CorrectExtnsnsArray[3], var4CorrectExtnsnsArray[4], var4CorrectExtnsnsArray[5],
                 var4TmprDrctry);
        system(var4Cmnd);

        // Check to see if any files were copied or not.
        int var4FilesCopied = 0;
        DIR *var4PtrTempDrctry = opendir(var4TmprDrctry);
        if (var4PtrTempDrctry) {
            struct dirent *var4EntryPtr;
            while ((var4EntryPtr = readdir(var4PtrTempDrctry)) != NULL) {
                if (var4EntryPtr->d_type == DT_REG) {
                    var4FilesCopied = 1;
                    break;
                }
            }
            closedir(var4PtrTempDrctry);
        }

        if (var4FilesCopied) {
            // Make a tarball of your temp. dir.
            char var4TarFlCmnd[RPPMCRO_ioBufferLength];
            snprintf(var4TarFlCmnd, sizeof(var4TarFlCmnd), "tar czf temp.tar.gz %s", var4TmprDrctry);
            system(var4TarFlCmnd);
        }

        // Clears temp. direc.
        char var4RmvFlCmnd[RPPMCRO_ioBufferLength];
        snprintf(var4RmvFlCmnd, sizeof(var4RmvFlCmnd), "rm -r %s", var4TmprDrctry);
        system(var4RmvFlCmnd);

        exit(var4FilesCopied ? EXIT_SUCCESS : EXIT_FAILURE);
    } else {  // Parent pr0cces
        int varSttsFlag;
        waitpid(var4ChldPrcID, &varSttsFlag, 0);  // Wait 4 child process to be over

        if (WIFEXITED(varSttsFlag) && WEXITSTATUS(varSttsFlag) == EXIT_SUCCESS) {
            // Send the client a msg .of success.
            const char *var4AcknRespMsg = "Permissible Files extensions and getting appropriate FileSize condition & Then stored in temp.tar.gz";
            send(param4ClntSckt, var4AcknRespMsg, strlen(var4AcknRespMsg), 0);
        } else {
            // Send the client an error ack.
            var4ErrAck = true;
            const char *var4AcknRespMsg = "No Files Found Under the range of the sizes given by client";
            send(param4ClntSckt, var4AcknRespMsg, strlen(var4AcknRespMsg), 0);
        }
    }
}

// Now for this func. it looks in the user's home directory for a file with a specified name and sends the findings to a client socket.

void func4FileSrchCmndExecution(int param4ClntSckt, const char *var4NameOfFile) {
    int var4PipeFileDesc[2];
    if (pipe(var4PipeFileDesc) == -1) {
        perror("***Failing of Pipe Operation***");
        return;
    }

    pid_t var4ChldPrcID = fork();
    if (var4ChldPrcID == -1) {
        perror("***Failing of Fork Operation***");
        close(var4PipeFileDesc[0]);
        close(var4PipeFileDesc[1]);
        return;
    }

    if (var4ChldPrcID == 0) { // Child pr0cces
        close(var4PipeFileDesc[0]); // Close read end of pipe

        // Redir stdout to pipe's write end.
        if (dup2(var4PipeFileDesc[1], STDOUT_FILENO) == -1) {
            perror("***Failing of 'dup2' Operation***");
            close(var4PipeFileDesc[1]);
            exit(EXIT_FAILURE);
        }

        // Using find cmnd for finding.
        execlp("find", "find", getenv("HOME"), "-name", var4NameOfFile, "-printf", "%p %s %TY-%Tm-%Td %TH:%TM:%TS\\n", NULL);

        //function creates a pipe to redirect the standard output of the find command to the parent process.
        //It then forks a child process, where the find command is executed with the specified file name. 
        perror("***Failing of 'execlp' Operation***"); // Execfailed:(
        close(var4PipeFileDesc[1]);
        exit(EXIT_FAILURE);
    } else { // Parent pr0cces
        close(var4PipeFileDesc[1]); //Close the pipe's write end

        char var4OutptBfr[RPPMCRO_ioBufferLength];
        memset(var4OutptBfr, 0, sizeof(var4OutptBfr));
        ssize_t var4TotalBytes2Read = read(var4PipeFileDesc[0], var4OutptBfr, sizeof(var4OutptBfr) - 1);

        if (var4TotalBytes2Read == -1) {
            perror("***Failing of 'read' Operation***");
            close(var4PipeFileDesc[0]);
            return;
        }

        close(var4PipeFileDesc[0]);

        if (var4TotalBytes2Read > 0) {
//The parent process parses the search command output to determine the file's path, size, and last modification date and time.


            char *var4PathOfFile = strtok(var4OutptBfr, " ");
            if (var4PathOfFile) {
                char *var4SizeOfFileToken = strtok(NULL, " ");
                char *var4DateOfFileToken = strtok(NULL, " ");
                char *var4TimeOfFileToken = strtok(NULL, "\n");
                
                char var4RspnseBfr[RPPMCRO_ioBufferLength];
                snprintf(var4RspnseBfr, sizeof(var4RspnseBfr), "File found:\nPath: %s\nSize: %s bytes\nDate: %s Time: %s\n", var4PathOfFile, var4SizeOfFileToken, var4DateOfFileToken, var4TimeOfFileToken);
                
                send(param4ClntSckt, var4RspnseBfr, strlen(var4RspnseBfr), 0);
            } else {  //This data is subsequently transmitted to the client socket. If the file cannot be located, the procedure returns to the client socket with a "File not found" error message.


                const char *var4NotFindResponse = "File not found\n";
                send(param4ClntSckt, var4NotFindResponse, strlen(var4NotFindResponse), 0);
            }
        } else {
            //Errors such as failing to build a pipe, failing to fork a child process, failing to duplicate file descriptors, 
            //failing to run the search command, and failing to read from the pipe are handled by this function
            //If error occurs, it displays an error message to the standard error stream and exits the function.a
            const char *var4NotFindResponse = "File not found\n";
            send(param4ClntSckt, var4NotFindResponse, strlen(var4NotFindResponse), 0);
        }
    }
}



void func4TargzfCmndExecution(int param4ClntSckt, const char *param4Cmnd) {
    // Extract the extension list and the -u flag from the command.

    char var4NameOfExtnsn[RPPMCRO_ioBufferLength];
    int var4UnzipFlag = 0;
    sscanf(param4Cmnd, "targzf %[^\n] %*[-u]", var4NameOfExtnsn);

    // Tokenise the ext. lest
    char *var4ExtnsnTkn = strtok(var4NameOfExtnsn, " ");
    int var4TotalExtnsnCnt = 0;

    // Make a temporary directory to store files in.
    char var4TmprDrctry[] = "temp_files";
    mkdir(var4TmprDrctry, 0700);

    // Fork Child pr0cces
    pid_t var4ChldPrcID = fork();

    if (var4ChldPrcID == -1) {
        perror("***Failing of Fork Operation***");
        return;
    }

    if (var4ChldPrcID == 0) {  // Child pr0cces
        // client socket should be redir to stdout
        dup2(param4ClntSckt, STDOUT_FILENO);

        // Search/Travers the $HOME directory tree for files with matching extensions.

        while (var4ExtnsnTkn != NULL && var4TotalExtnsnCnt < 4) {
            char var4Cmnd[RPPMCRO_ioBufferLength];
            snprintf(var4Cmnd, sizeof(var4Cmnd), "find $HOME -name '*.%s' -type f -exec cp {} %s/ \\;", var4ExtnsnTkn, var4TmprDrctry);
            system(var4Cmnd);
            var4TotalExtnsnCnt++;
            var4ExtnsnTkn = strtok(NULL, " ");
        }

        // Determine whether any files were copied.

        int var4FilesCopied = 0;
        DIR *var4PtrTempDrctry = opendir(var4TmprDrctry);
        if (var4PtrTempDrctry) {
            struct dirent *var4EntryPtr;
            while ((var4EntryPtr = readdir(var4PtrTempDrctry)) != NULL) {
                if (var4EntryPtr->d_type == DT_REG) {
                    var4FilesCopied = 1;
                    break;
                }
            }
            closedir(var4PtrTempDrctry);
        }

        if (var4FilesCopied) {
            // Make a tarball of your temp. dir.
            char var4TarFlCmnd[RPPMCRO_ioBufferLength];
            snprintf(var4TarFlCmnd, sizeof(var4TarFlCmnd), "tar czf temp.tar.gz %s", var4TmprDrctry);
            system(var4TarFlCmnd);
        }

        // Clears temp. direc.
        char var4RmvFlCmnd[RPPMCRO_ioBufferLength];
        snprintf(var4RmvFlCmnd, sizeof(var4RmvFlCmnd), "rm -r %s", var4TmprDrctry);
        system(var4RmvFlCmnd);

        exit(var4FilesCopied ? EXIT_SUCCESS : EXIT_FAILURE);
    } else {  // Parent pr0cces
        int varSttsFlag;
        waitpid(var4ChldPrcID, &varSttsFlag, 0);  // Await for child process to complete

        if (WIFEXITED(varSttsFlag) && WEXITSTATUS(varSttsFlag) == EXIT_SUCCESS) {
            // informs succes ackt. to client
            const char *var4AcknRespMsg = "Given Permissible Files extensions & Then stored in temp.tar.gz";
            send(param4ClntSckt, var4AcknRespMsg, strlen(var4AcknRespMsg), 0);
        } else {
            // informs erorr ackt. to clien
            var4ErrAck = true;
            const char *var4AcknRespMsg = "No Files Found for the given extensions";
            send(param4ClntSckt, var4AcknRespMsg, strlen(var4AcknRespMsg), 0);
        }
    }
}




void func4GetdirfCmndExecution(int param4ClntSckt, const char *param4Cmnd) {
    // Create variables to hold the date arguments.

    char var4DateArg1[RPPMCRO_ioBufferLength];
    char var4DateArg2[RPPMCRO_ioBufferLength];
    //int var4UnzipFlag = 0;
    //To obtain the date arguments, parsin cmnd.
    sscanf(param4Cmnd, "getdirf %s %s %*[-u]", var4DateArg1, var4DateArg2);
    // Declare and initialise date argument tm structs

    struct tm var4DateTime1 = {0};
    struct tm var4DateTime2 = {0};
    strptime(var4DateArg1, "%Y-%m-%d", &var4DateTime1);
    strptime(var4DateArg2, "%Y-%m-%d", &var4DateTime2);
    time_t varDateR1 = mktime(&var4DateTime1);
    time_t varDateR2 = mktime(&var4DateTime2);

    // make temp dir
    char var4TmprDrctry[] = "temp_files";
    mkdir(var4TmprDrctry, 0700);

    // Fork Child pr0cces
    pid_t var4ChldPrcID = fork();

    // sees for failure in fork
    if (var4ChldPrcID == -1) {
        perror("***Failing of Fork Operation***");
        return;
    }

    if (var4ChldPrcID == 0) {  // Child pr0cces
        // Redirect stdout to the client socket.
        dup2(param4ClntSckt, STDOUT_FILENO);

        size_t varSizeOfCmnd = strlen("find $HOME -type f -newerBt '' ! -newerBt '' -exec cp {} / \\;") +
                          strlen(var4DateArg1) + strlen(var4DateArg2) + strlen(var4TmprDrctry) + 1;

        char *var4Cmnd = (char *)malloc(varSizeOfCmnd);
        if (var4Cmnd == NULL) {
            //sees for malloc failure
            perror("***Failing of 'malloc' Operation***");
            exit(EXIT_FAILURE);
        }

        snprintf(var4Cmnd, varSizeOfCmnd, "find $HOME -type f -newermt '%s' ! -newermt '%s' -exec cp {} %s/ \\;", var4DateArg1, var4DateArg2, var4TmprDrctry);

        // Exec. the find command
        system(var4Cmnd);

        int var4FilesCopied = 0;
        DIR *var4PtrTempDrctry = opendir(var4TmprDrctry);
        if (var4PtrTempDrctry) {
            struct dirent *var4EntryPtr;
            while ((var4EntryPtr = readdir(var4PtrTempDrctry)) != NULL) {
                if (var4EntryPtr->d_type == DT_REG) {
                    var4FilesCopied = 1;
                    break;
                }
            }
            closedir(var4PtrTempDrctry);
        }
            // Archive the files in a tar.gz if any files were copied
        if (var4FilesCopied) {
            char var4TarFlCmnd[RPPMCRO_ioBufferLength];
            snprintf(var4TarFlCmnd, sizeof(var4TarFlCmnd), "tar czf temp.tar.gz %s", var4TmprDrctry);
            system(var4TarFlCmnd);
        }

        char var4RmvFlCmnd[RPPMCRO_ioBufferLength];
        snprintf(var4RmvFlCmnd, sizeof(var4RmvFlCmnd), "rm -r %s", var4TmprDrctry);
        system(var4RmvFlCmnd);

        //free memory alloc for find cmnd.
        free(var4Cmnd);
        exit(var4FilesCopied ? EXIT_SUCCESS : EXIT_FAILURE);
    } else {  // Parent pr0cces
        int varSttsFlag;
        waitpid(var4ChldPrcID, &varSttsFlag, 0);  // Wait for the child process to finish

        // Checks child process was successful or not
        if (WIFEXITED(varSttsFlag) && WEXITSTATUS(varSttsFlag) == EXIT_SUCCESS) {
            // Send success acknowledgment to the client
            const char *var4AcknRespMsg = "Appropriate Files under the given range of dates copiied & then stored in temp.tar.gz";
            send(param4ClntSckt, var4AcknRespMsg, strlen(var4AcknRespMsg), 0);
        } else {
            // Send error acknowledgment to the client
            var4ErrAck = true;
            const char *var4AcknRespMsg = "No Fiiles Found under the specified range of dates";
            send(param4ClntSckt, var4AcknRespMsg, strlen(var4AcknRespMsg), 0);
        }
    }
}



void func4ClientOpsExecution(int param4ClntSckt) {
    char varBfr[RPPMCRO_ioBufferLength];
    ssize_t var4ResultantBytesGot;
    bool var4PrgrmQuitFlag = false;
    
    // Loop until program quit flag is set
    while (!var4PrgrmQuitFlag) {
        var4ErrAck = false;
        // Receive command from client
        memset(varBfr, 0, sizeof(varBfr));
        var4ResultantBytesGot = recv(param4ClntSckt, varBfr, sizeof(varBfr), 0);
        if (var4ResultantBytesGot <= 0) {
            perror("***Failing of recvCommand***");
            break;
        }
        
        printf("**Command sent by Client- %s\n", varBfr);

        // Handling different commands
        if (strncmp(varBfr, "fgets", 5) == 0) {
            func4FgetsCmndExecution(param4ClntSckt, varBfr);
        } 
        // Handle temp_tar command if error ack is not set
        else if ((strncmp(varBfr, "temp_tar", 8)) == 0 && var4ErrAck == false) {
		func4TransferTempTarFile(param4ClntSckt);
    	} 
        // Handle tarfgetz command
    	else if (strncmp(varBfr, "tarfgetz", 8) == 0) {
            func4TarfgetzCmndExecution(param4ClntSckt, varBfr);
        } 
        // Handle filesrch command
        else if (strncmp(varBfr, "filesrch", 8) == 0) {
        	char var4NameOfFile[RPPMCRO_ioBufferLength];
		sscanf(varBfr, "filesrch %s", var4NameOfFile);
		func4FileSrchCmndExecution(param4ClntSckt, var4NameOfFile);
        } 
        // Handle targzf command
        else if (strncmp(varBfr, "targzf", 6) == 0) {
            func4TargzfCmndExecution(param4ClntSckt, varBfr);
         } 
         //getdirf
         else if (strncmp(varBfr, "getdirf", 7) == 0) {
            func4GetdirfCmndExecution(param4ClntSckt, varBfr);
         }
         //quit
         else if (strncmp(varBfr, "quit", 4) == 0) {
            var4PrgrmQuitFlag = true;
         }
         else {
            const char *var4AcknRespMsg = "Incorrect type of Cmnd Given";
            send(param4ClntSckt, var4AcknRespMsg, strlen(var4AcknRespMsg), 0);
        }
    }
    // Close the client socket
    close(param4ClntSckt);
}



int main() {
    int param4SrvrSckt, param4ClntSckt;
    struct sockaddr_in var4SrvrAddLoc, var4ClntAddLoc;
    socklen_t var4SizeOfClntAddLoc = sizeof(var4ClntAddLoc);
    int var4ReeUseFlag = 1;  // Set the SO_REUSEADDR option
    int var4ClntCounter = 0;

    // Create socket
    if ((param4SrvrSckt = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("***Failing of 'S0cket' Operation***");
        exit(EXIT_FAILURE);
    }

    // Set the SO_REUSEADDR option
    if (setsockopt(param4SrvrSckt, SOL_SOCKET, SO_REUSEADDR, &var4ReeUseFlag, sizeof(int)) == -1) {
        perror("***Failing of 'setsockopt' Operation***");
        exit(EXIT_FAILURE);
    }

    // Set up server address structure
    var4SrvrAddLoc.sin_family = AF_INET;
    var4SrvrAddLoc.sin_port = htons(RPPMCRO_cncPort4Server);
    var4SrvrAddLoc.sin_addr.s_addr = INADDR_ANY;

    // Bind
    if (bind(param4SrvrSckt, (struct sockaddr *)&var4SrvrAddLoc, sizeof(var4SrvrAddLoc)) == -1) {
        perror("***Failing of 'bind' Operation***");
        exit(EXIT_FAILURE);
    }

    // Listen
    if (listen(param4SrvrSckt, RPPMCRO_maxPermClients) == -1) {
        perror("***Failing of 'listen' Operation***");
        exit(EXIT_FAILURE);
    }

    printf("**Server Active for Clients Connection on the port: %d...\n", RPPMCRO_cncPort4Server);

    while (1) {
        // Accept incoming connection
        if ((param4ClntSckt = accept(param4SrvrSckt, (struct sockaddr *)&var4ClntAddLoc, &var4SizeOfClntAddLoc)) == -1) {
            perror("***Failing of 'accept' Operation***");
            continue;
        }
        var4ClntCounter++;
        int var4ClntTag = 0;
        // Identify if the client is greater than 12
        if(var4ClntCounter > 12){
        var4ClntTag = var4ClntCounter % 2;
        }
        // Sending Client counter to client
        send(param4ClntSckt, &var4ClntCounter, sizeof(var4ClntCounter), 0);
        
    // Determine whether the client counter is less than or equal to 6 or whether the client tag is 1.

        if (var4ClntCounter <= 6 || var4ClntTag == 1){
        send(param4ClntSckt, "SERVER", 6, 0);
        printf("**Establishment of New Client's Connection with %s:%d\n", inet_ntoa(var4ClntAddLoc.sin_addr), ntohs(var4ClntAddLoc.sin_port));
        printf("**Client: %d\n", var4ClntCounter);
        // Fork a new process to handle the client
        pid_t prcId = fork();
        if (prcId < 0) {
            perror("***Failing of 'fork' Operation***");
            continue;
        } else if (prcId == 0) {
            // Child pr0cces
            close(param4SrvrSckt); // Close the server socket in the child
            func4ClientOpsExecution(param4ClntSckt);
            exit(0);  // Child pr0cces should exit when done handling the client
        } else {
            // Parent pr0cces
            close(param4ClntSckt); // Close the client socket in the parent
        }
       }
       // Determine whether the client counter is higher than 6 and less than or equal to 12, or whether the client tag is 0.
       else if((var4ClntCounter > 6 && var4ClntCounter <=12) || var4ClntTag == 0){
     send(param4ClntSckt, "MIRROR", 6, 0);
     }
     }
     
     // Closing the server socket
    close(param4SrvrSckt);
    return 0;
}

