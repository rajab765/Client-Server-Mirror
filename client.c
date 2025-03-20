//***Advanced Systems Programming--Project***

//***Below are the appropriate header files used in the program***
//***The below line of code includes headerFile for stndrd Inp/Op library   
#include <stdio.h>
//***The below line of code includes headerFile for stndrd library for different functions like m/m  
#include <stdlib.h>
//***The below line of code includes headerFile for string manipulation, m/m copying, string cmp
#include <string.h>
//***The below line of code includes headerFile for accessing multiple stndrd POSIX funcs
#include <unistd.h>
//***The below line of code includes headerFile for handling internet addresses and IP addresses
#include <arpa/inet.h>
//***The below line of code includes headerFile for executing regular expressions
#include <regex.h>
//***The below line of code includes headerFile for handling chrcter classification and cnversions
#include <ctype.h>
//***The below line of code includes headerFile for implementing bool type
#include <stdbool.h>
//***The below line of code includes headerFile for implementing process controls and waiting for chld prcess
#include <sys/wait.h>

//Initialising macro RPPMCRO_localHostIP
#define RPPMCRO_localHostIP "127.0.0.1"
//Initialising macro RPPMCRO_cncPort4Server
#define RPPMCRO_cncPort4Server 6312
//Initialising macro RPPMCRO_cncPort4Mirror
#define RPPMCRO_cncPort4Mirror 6313
//Initialising macro RPPMCRO_ioBufferLength
#define RPPMCRO_ioBufferLength 2048
//Initialising macro RPPMCRO_MaxPermissibleExtnsns
#define RPPMCRO_MaxPermissibleExtnsns 4

//Declaring and assigning varibale var4UOptCheck for unzip option in cmnd
bool var4UOptCheck = false;

// This func. unzips temp.tar.gz file. Here the fork func is used that creates child pr0cces now it'll executes tar cmnd for unzipping file.
//Atlast pr0cces awaits for child to get done.
void func4UnzipOfTempFileTar() {
    pid_t var4ChldPrcID = fork();
    if (var4ChldPrcID == -1) {
        perror("***Failing of Fork Operation***");
        return;
    } else if (var4ChldPrcID == 0) {
        // Child process (tar command)
        execlp("tar", "tar", "-xzf", "temp.tar.gz", NULL);
        perror("***Failing of execlp Operation***");
        exit(EXIT_FAILURE);
    } else {
        // Parent process
        int varSttsFlag;
        waitpid(var4ChldPrcID, &varSttsFlag, 0);
        if (WIFEXITED(varSttsFlag) && WEXITSTATUS(varSttsFlag) != 0) {
            printf("***Failing of File Operation for unzip it***\n");
        }
    }
}


//Now it will receive the tar that we unzipped inn the func4UnzipOfTempFileTar() funct i.e. from the server and saves locally.
//firstly, it gets the ack. from server side, followed by file size and lastly file itself, then saved in binary mode.
void func4gettingTempFileTar(int param4SrvrSocket) {
    char varBfr[RPPMCRO_ioBufferLength];
    ssize_t var4ResultantBytesGot;

    // Receive acknowledgment
    memset(varBfr, 0, sizeof(varBfr));
    var4ResultantBytesGot = recv(param4SrvrSocket, varBfr, sizeof(varBfr), 0);
    if (var4ResultantBytesGot <= 0) {
        perror("***Failing of Getting TempTar file Operation***");
        return;
    }

    if (strcmp(varBfr, "TEMP_TAR_READY") != 0) {
        printf("***Failing of Getting the **TEMP_TAR_READY** accknwlgmnt from the server***\n");
        return;
    }

    // Receive file size
    size_t var4LengthOfFile;
    var4ResultantBytesGot = recv(param4SrvrSocket, &var4LengthOfFile, sizeof(size_t), 0);
    if (var4ResultantBytesGot <= 0) {
        perror("***Failing of recvCommand***");
        return;
    }

    // Receive and save the file
    FILE *varFilePtr = fopen("temp.tar.gz", "wb");
    if (varFilePtr == NULL) {
        perror("***Failing of open Operation of File***");
        return;
    }

    size_t var4LeftBytes = var4LengthOfFile;
    while (var4LeftBytes > 0) {
        memset(varBfr, 0, sizeof(varBfr));
        var4ResultantBytesGot = recv(param4SrvrSocket, varBfr, sizeof(varBfr), 0);
        if (var4ResultantBytesGot <= 0) {
            perror("recv");
            break;
        }

        fwrite(varBfr, 1, var4ResultantBytesGot, varFilePtr);
        var4LeftBytes -= var4ResultantBytesGot;
    }

    fclose(varFilePtr);
    printf("***Successful execution of getting the tempTar file from the server and locally stored***\n");
}



//validates syntax of "fgets" cmnd. What it does is thaqt the cmnd starts with fgets and has filname between 1&4
int func4ChkingCorrectCmndOfFgets(const char *param4Cmnd) {
    
    if (strncmp(param4Cmnd, "fgets", 5) != 0) {
        return 0;
    }

    // Counts no. exactly after fgets
    const char *var4NameOfFilesPtr = param4Cmnd + 5;
    int var4TotalFls = 0;
    while (*var4NameOfFilesPtr == ' ') {
        ++var4NameOfFilesPtr;
    }
    while (*var4NameOfFilesPtr != '\0') {
        if (*var4NameOfFilesPtr != ' ') {
            ++var4TotalFls;
            while (*var4NameOfFilesPtr != ' ' && *var4NameOfFilesPtr != '\0') {
                ++var4NameOfFilesPtr;
            }
        } else {
            while (*var4NameOfFilesPtr == ' ') {
                ++var4NameOfFilesPtr;
            }
        }
    }
    return (var4TotalFls >= 1 && var4TotalFls <= 4);
}

// Extrcts args. after from "tarfgetz" and checks if numeruc and also for the optional -u option.
int func4ChkingCorrectCmndOfTarfgetz(const char *param4Cmnd) {
    // Validate the tarfgetz command syntax here
    if (strncmp(param4Cmnd, "tarfgetz", 8) != 0) {
        return 0;
    }

    
    const char *var4Parameters = param4Cmnd + 8;
    char param1[16], param2[16], varCmndOpt[3];
    int varFinalRes = sscanf(var4Parameters, "%s %s %2s", param1, param2, varCmndOpt);
    if (varFinalRes < 2) {
        return 0;
    }

    // here it sees args. are numeric
    for (int k = 0; k < strlen(param1); ++k) {
        if (!isdigit(param1[k])) {
            return 0;
        }
    }
    for (int k = 0; k < strlen(param2); ++k) {
        if (!isdigit(param2[k])) {
            return 0;
        }
    }

    // here it checks for -u option
    if (varFinalRes == 3) {
        if (strcmp(varCmndOpt, "-u") != 0) {
            return 0;
        }
        // Set var4UOptCheck to true if -u option is present
        var4UOptCheck = true;
    }

    // Now here it looks for sizetwo is greater than sizeone
    return atoi(param2) >= atoi(param1);
}


// this func validates syntax of filesearch cmnd  
int func4ChkingCorrectCmndOfFileSearch(const char *param4Cmnd) {
    // checks if the cmnd string "param4cmnd" starts wth filesrch using funct. strncmp.
    //If not returns 0
    if (strncmp(param4Cmnd, "filesrch", 8) != 0) {
        return 0;
    }

    // Skips whitespaces following the filesrch prefix
    const char *var4NameOfFilesPtr = param4Cmnd + 8;
    while (*var4NameOfFilesPtr == ' ') {
        ++var4NameOfFilesPtr;
    }

    // Check valid ext. by usinng strstr
    const char *var4CorrectExtnsnsArray[] = {"txt", "pdf", "png", "jpg", "jpeg", "c"};
    for (int k = 0; k < sizeof(var4CorrectExtnsnsArray) / sizeof(var4CorrectExtnsnsArray[0]); ++k) {
        if (strstr(var4NameOfFilesPtr, var4CorrectExtnsnsArray[k])) {
            // Sees only 1 filename is provided andchecks if spaces there anf if found one returns zero
            while (*var4NameOfFilesPtr != '\0') {
                if (*var4NameOfFilesPtr == ' ') {
                    return 0;
                }
                ++var4NameOfFilesPtr;
            }
            return 1;
        }
    }
    // if reaches here it means func has no valid ext. i.e invalid cmnd
    return 0;
}

//this functions extracts the ext. and sees if -u present
int func4ChkingCorrectCmndOfTargzf(const char *param4Cmnd) {
    // Check if the command starts with "targzf"
    if (strncmp(param4Cmnd, "targzf", 6) != 0)
        return 0;

    // Move the pointer past "targzf"
    param4Cmnd += 6;

    // Skip spaces after "targzf"
    while (*param4Cmnd == ' ')
        param4Cmnd++;

    bool var4ChckExtnsnIncluded = false;
    int var4TotalExtn = 0;

    // Process extension list and optional -u flag
    while (*param4Cmnd != '\0') {
        // Check for optional -u flag
        if (strncmp(param4Cmnd, "-u", 2) == 0) {
            var4UOptCheck = true;
            param4Cmnd += 2;

            // Skip spaces after -u
            while (*param4Cmnd == ' ')
                param4Cmnd++;
        }

        if (*param4Cmnd != '\0') {
            var4TotalExtn++;

            // Read extension or file type
            while (*param4Cmnd != '\0' && *param4Cmnd != ' ') {
                if (*param4Cmnd == '-') {
                    // Skip to the next extension or file type
                    while (*param4Cmnd != '\0' && *param4Cmnd != ' ')
                        param4Cmnd++;
                } else {
                    var4ChckExtnsnIncluded = true;

                    // Read the extension or file type
                    while (*param4Cmnd != '\0' && *param4Cmnd != ' ')
                        param4Cmnd++;
                }
            }

            // Skip spaces after extension or file type
            while (*param4Cmnd == ' ')
                param4Cmnd++;
        }
    }

    // Validate the command format
    if (var4TotalExtn >= 1 && var4TotalExtn <= RPPMCRO_MaxPermissibleExtnsns)
        return var4UOptCheck ? 2 : 1;

    return 0;
}

// //this functions extracts the ext. and sees if -u present
// int func4ChkingCorrectCmndOfTargzf(const char *param4Cmnd) {
//     int varCmndlngth = strlen(param4Cmnd);
//     if (varCmndlngth < 10) {  // must be atlest 10char long if not returns zero
//         return 0;  // Notvalid cmnd length
//     }
    
//     if (strncmp(param4Cmnd, "targzf ", 7) != 0) {
//         return 0;  // Notvalid cmnd format
//     }
//     //function uses the strstr function for check if the "-u" option is inclde in the command. If it found, the global variable var4UOptCheck are set to true.
//     const char *var4BeginExtnLest = param4Cmnd + 7;
//     const char *var4UnzipChck = strstr(var4BeginExtnLest, " -u");
    
//     //check if ext list provided before the "-u" option. If the "-u" option is present and no ext. before it, the function return 0, indicate invalid command
//     if (var4UnzipChck && var4UnzipChck - var4BeginExtnLest <= 1) {
//         return 0;  
//     }
    
    
//     if (var4UnzipChck) {
//         var4UOptCheck = true;
//     }
//     //Count Ext.
//     //Ext. are also seperated by spaces
//     const char *var4FinishExtnLest= var4UnzipChck ? var4UnzipChck : param4Cmnd + varCmndlngth;
//     int var4TotalExtn = 0;
//     const char *var4Extn = var4BeginExtnLest;
    
//     while (var4Extn < var4FinishExtnLest) {
//         if (*var4Extn != ' ') {
//             var4TotalExtn++;
//             while (*var4Extn != ' ' && var4Extn < var4FinishExtnLest) {
//                 var4Extn++;
//             }
//         }
//         var4Extn++;
//     }
    
//     //only returns if no. between 1&4, if not returns zero
//     if (var4TotalExtn < 1 || var4TotalExtn > 4) {
//         return 0;  // Invalid number of extensions
//     }
    
//     return var4UOptCheck ? 2 : 1;  // 1=valid command, 2=-u flag there
// }


int func4ChkingCorrectDate(const char *arg4Date) {
    // Validaes date'sformat as "YYYY-MM-DD"
    regex_t var4Reegex;
    int var4Reslt;
    char var4RegexPatern[] = "^[0-9]{4}-(0[1-9]|1[0-2])-([0-2][0-9]|3[0-1])$";
    
    var4Reslt = regcomp(&var4Reegex, var4RegexPatern, REG_EXTENDED);
    //compiles reg. exp. if error returns0
    if (var4Reslt) {
        fprintf(stderr, "***Failed to process the regular expression***\n");
        return 0;
    }
    
    if (regexec(&var4Reegex, arg4Date, 0, NULL, 0)) {
        regfree(&var4Reegex);
        return 0;
    }
    //if matches patern, frees memory alloc. to reg exp. and returns1., otherwise 0(above)
    regfree(&var4Reegex);
    return 1;
}

//Now what it does is This method checks the syntax of the getdirf command. 
//It analyzes the inputs returned by "getdirf" to see if the dates are correct and if the optional -u option is included.
int func4ChkingCorrectCmndOfGetdirf(const char *param4Cmnd) {
    // Validte cmnd here
    if (strncmp(param4Cmnd, "getdirf", 7) != 0) {
        return 0;
    }

    // Extr. args. after the "getdirf"
    const char *var4Parameters = param4Cmnd + 7;
    char var4DateArg1[16], var4DateArg2[16], var4OptArray[3];
    
    // To prevent potential issues, we intentionaly initialize the option to an empty string.
    var4OptArray[0] = '\0';
    
    int var4FinalReslt = sscanf(var4Parameters, "%s %s %2s", var4DateArg1, var4DateArg2, var4OptArray);
    if (var4FinalReslt < 2) {
        return 0;
    }

    // Sees if -u opt.detected
    if (var4FinalReslt == 3) {
        if (strcmp(var4OptArray, "-u") == 0) {
            var4UOptCheck = true;
        } else {
            
            return 0;
        }
    }

    // Validate values of date and month
    if (!func4ChkingCorrectDate(var4DateArg1) || !func4ChkingCorrectDate(var4DateArg2)) {
        return 0;
    }

    return 1;
}




int func4ChkingCorrectCmndOfQuit(const char *param4Cmnd) {
    // Validte quit cmmnd
    return (strcmp(param4Cmnd, "quit") == 0);
}



// Overall what the main func does is This code creates a client that establishes a connection to a server using a socket. 
//It then sends commands to the server and waits for the responses. If the server replies with "MIRROR", 
//the client takes care of setting up a mirror connection. The client continually processes commands in an endless loop until
//it receives the "quit" command. Additionally, it verifies the syntax of the commands, receives the server's feedback, and acts accordingly.

int main() {
    // Declare var for client socket, server and mirror port details, buffer, received bytes, and flags

    int var4ClientWSocketCom;
    struct sockaddr_in var4ServerPortDetails;
    struct sockaddr_in var4MirrorPortDetails;
    char varBfr[RPPMCRO_ioBufferLength];
    ssize_t var4ResultantBytesGot;
    bool var4CheckingTemp = false;
    bool var4NotFindFlag = false;
    int var4ClntCounter = 0;
    
    // Create socket
    var4ClientWSocketCom = socket(AF_INET, SOCK_STREAM, 0);

    // Sets up server port details
    var4ServerPortDetails.sin_family = AF_INET;
    var4ServerPortDetails.sin_port = htons(RPPMCRO_cncPort4Server);
    var4ServerPortDetails.sin_addr.s_addr = inet_addr(RPPMCRO_localHostIP);
    
    // Sets up mirror port detail
    //memset(&var4MirrorPortDetails, 0, sizeof(var4MirrorPortDetails));
    var4MirrorPortDetails.sin_family = AF_INET;
    var4MirrorPortDetails.sin_port = htons(RPPMCRO_cncPort4Mirror); 
    var4MirrorPortDetails.sin_addr.s_addr = inet_addr(RPPMCRO_localHostIP);
    
    // Connect to the server
    connect(var4ClientWSocketCom, (struct sockaddr *)&var4ServerPortDetails, sizeof(var4ServerPortDetails));
    //connect(var4ClientWSocketCom, (struct sockaddr *)&var4MirrorPortDetails, sizeof(var4MirrorPortDetails));
    
    // Receiving client counter from server 
    recv(var4ClientWSocketCom, &var4ClntCounter, sizeof(var4ClntCounter), 0);
    //printf("Counter: %d \n", var4ClntCounter);
    
    
    // Initialize the buffer and receive data from server
    memset(varBfr, 0, sizeof(varBfr));
    recv(var4ClientWSocketCom, varBfr, sizeof(varBfr), 0);
    //printf("%s\n\n", varBfr);
    
        // Check if the server responds with "MIRROR" or "SERVER"
    	if(strcmp(varBfr, "MIRROR") == 0){
		close(var4ClientWSocketCom);
    		var4ClientWSocketCom = socket(AF_INET, SOCK_STREAM, 0);
    		connect(var4ClientWSocketCom, (struct sockaddr *)&var4MirrorPortDetails, sizeof(var4MirrorPortDetails));
    		// Sending counter to mirror
    		send(var4ClientWSocketCom, &var4ClntCounter, sizeof(var4ClntCounter), 0);
    		printf("***Successful Establishment of Mirror Connection***\n");
    	}
    	else if(strcmp(varBfr, "SERVER") == 0){
    	printf("***Successful Establishment of Server Connection***\n");
    	}
    	// loop which is Infinite is for receiving and processing user commands

    while (1) {
    	var4UOptCheck = false;
        var4CheckingTemp = false;
        var4NotFindFlag =false;
        
        printf("\nC$ ");
        fgets(varBfr, sizeof(varBfr), stdin);
        //printf("\n%d\n", func4ChkingCorrectCmndOfTargzf(varBfr));
        
        // Remove trailing freshline char from input
        size_t var4StrngLngth = strlen(varBfr);
        if (var4StrngLngth > 0 && varBfr[var4StrngLngth - 1] == '\n') {
            varBfr[var4StrngLngth - 1] = '\0';
        }
        
        
        // Validats cmnd and sends to server
        if (func4ChkingCorrectCmndOfQuit(varBfr)) {
            // Exits loop after Quit cmnd is been send to the server
            send(var4ClientWSocketCom, varBfr, strlen(varBfr), 0);
            break;
        } else if (!func4ChkingCorrectCmndOfFgets(varBfr) && !func4ChkingCorrectCmndOfTarfgetz(varBfr) && !func4ChkingCorrectCmndOfFileSearch(varBfr) && !func4ChkingCorrectCmndOfTargzf(varBfr) && !func4ChkingCorrectCmndOfGetdirf(varBfr)) {
            printf("***Incorrect syntax of the command given. Kindly give the correct [fgets, filesrch, tarfgetz,  targzf, getdirf & quit ] commands with arguements ***\n");
            continue;
        }
        if (func4ChkingCorrectCmndOfFgets(varBfr) || func4ChkingCorrectCmndOfTarfgetz(varBfr) || func4ChkingCorrectCmndOfTargzf(varBfr) || func4ChkingCorrectCmndOfGetdirf(varBfr)){
        var4CheckingTemp = true;
        }

        // cmnd is been send to server
        send(var4ClientWSocketCom, varBfr, strlen(varBfr), 0);

        // Receive and display the response from the server
        memset(varBfr, 0, sizeof(varBfr));
        var4ResultantBytesGot = recv(var4ClientWSocketCom, varBfr, sizeof(varBfr), 0);
        if (var4ResultantBytesGot <= 0) {
            perror("***Failing of recvCommand***");
            break;
        }

        printf("***The Response/Accknwlgment from the Server- %s\n", varBfr);
        // Check if no files were found
        if(strncmp(varBfr, "No_files_found", 14) == 0){
        var4NotFindFlag = true;
        }
        if(var4CheckingTemp && !var4NotFindFlag){
        // Send a command to request the temp.tar.gz file
    	send(var4ClientWSocketCom, "temp_tar", 8, 0);

    	// Receive and save the temp.tar.gz file
    	func4gettingTempFileTar(var4ClientWSocketCom);
    	}
    	//printf("\n%d\n", var4UOptCheck);
    	if(var4UOptCheck){
        func4UnzipOfTempFileTar();
        }
    }
    close(var4ClientWSocketCom);
    return 0;
}