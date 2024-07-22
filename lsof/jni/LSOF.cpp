#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <PrintLog.h>

int main(int argc, char **argv)
{
    FILE *fp = NULL;
    char szMapFilePath[32] = {0};
    char szMapFileLine[1024] = {0};
    char szPrcMapFileLine[1024] = {0};

    sprintf(szMapFilePath, "lsof | grep %d | grep .so", getpid());
    LOGD("szMapFilePath: %s", szMapFilePath);


    fp = popen(szMapFilePath, "r");
    if (fp != NULL)
    {
        while (fgets(szMapFileLine, 1023, fp) != NULL)
        {
            LOGD("szMapFileLine: %s", szMapFileLine);
            if (strstr(szMapFileLine, "libLSOF.so")){
                memcpy(szPrcMapFileLine, szMapFileLine, strlen(szMapFileLine));
            }
        }

        fclose(fp);
    }

    LOGD("szPrcMapFileLine: %s", szPrcMapFileLine);

	return 0;
}
