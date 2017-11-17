#include<stdio.h> 
 #include<stdlib.h> 
 #include <sys/types.h> 
 #include <dirent.h> 
 #include <sys/stat.h> 
 #include <unistd.h> 
 #include <fcntl.h> 
 #include <sys/ioctl.h> 
  
 int main(void) 
 { 
    char *curr_dir = NULL; 
    DIR *dp = NULL; 
    struct dirent *dptr = NULL; 
    unsigned int count = 0; 
  
    // Find the column width of terminal 
    // We will make use of this in part-II  
    // Of this article. 
    // struct winsize w; 
    // ioctl(STDOUT_FILENO, TIOCGWINSZ, &w); 
  
    curr_dir = getenv("PWD"); 
    if(NULL == curr_dir) 
    { 
        return 1; 
    } 
     
    dp = opendir((const char*)curr_dir);    
    if(NULL == dp) 
    { 
    	return 2; 
    } 
   
    for(count = 0; NULL != (dptr = readdir(dp)); count++) 
    { 
        if(dptr->d_name[0] != '.') 
        { 
            printf("%s     ",dptr->d_name); 
        } 
    } 
    printf("\n"); 
  
    return 0; 
 }