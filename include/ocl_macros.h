#if !defined OCL_MACROS_H
#define OCL_MACROS_H

#define LOG_OCL_ERROR(x, STRING )  if(x!=CL_SUCCESS) {printf( "\nLine No: %d", __LINE__ ); printf(STRING); printf("Error= %d",x);}

#define LOG_OCL_COMPILER_ERROR(PROGRAM, DEVICE)            \
        {                                                                                \
            cl_int logStatus;                                                            \
            char * buildLog = NULL;                                                      \
            size_t buildLogSize = 0;                                                     \
            logStatus = clGetProgramBuildInfo(PROGRAM,                                   \
                                              DEVICE,                                    \
                                              CL_PROGRAM_BUILD_LOG,                      \
                                              buildLogSize,                              \
                                              buildLog,                                  \
                                              &buildLogSize);                            \
            if(logStatus != CL_SUCCESS)                                                  \
            {                                                                            \
                std::cout << "Error # "<< logStatus                                      \
                    <<":: clGetProgramBuildInfo<CL_PROGRAM_BUILD_LOG> failed.";          \
                exit(1);                                                                 \
            }                                                                            \
                                                                                         \
            buildLog = (char*)malloc(buildLogSize);                                      \
            if(buildLog == NULL)                                                         \
            {                                                                            \
                std::cout << "Failed to allocate host memory. (buildLog)\n";             \
                return -1;                                                               \
            }                                                                            \
            memset(buildLog, 0, buildLogSize);                                           \
                                                                                         \
            logStatus = clGetProgramBuildInfo(PROGRAM,                                   \
                                              DEVICE,                                    \
                                              CL_PROGRAM_BUILD_LOG,                      \
                                              buildLogSize,                              \
                                              buildLog,                                  \
                                              NULL);                                     \
            if(logStatus != CL_SUCCESS)                                                  \
            {                                                                            \
                std::cout << "Error # "<< logStatus                                      \
                    <<":: clGetProgramBuildInfo<CL_PROGRAM_BUILD_LOG> failed.";          \
                exit(1);                                                                 \
            }                                                                            \
                                                                                         \
            std::cout << " \n\t\t\tBUILD LOG\n";                                         \
            std::cout << " ************************************************\n";          \
            std::cout << buildLog << std::endl;                                          \
            std::cout << " ************************************************\n";          \
            free(buildLog);                                                              \
        } 


#endif