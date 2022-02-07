#include <stdio.h>
#include <stdint.h>

const char *data_path = "/sys/kernel/debug/Load_Step/data";

int main(int argc, char** argv)
{
    char *b = argv[1];
    int NUM_SETS = atoi(b);

    printf("NUM_SET %d \n", NUM_SETS);

    FILE *ifp;
    ifp = fopen(data_path, "rb");
    if (!ifp)
    {
        printf("[ERR] open file failed.\n");
        return 1;
    }

    FILE * fp;
    fp = fopen ("./data.csv","w");

    unsigned int rd = 1;
    int cnt=0, epoch=0;
    unsigned char buf_sets_8[NUM_SETS+1];
    unsigned char buf_temp_8[4096];

    fprintf(fp, "irq_epoch");
    for(int i=0; i<NUM_SETS; i++)
    {
        fprintf(fp, ",set_%d", i);
    }
    fprintf(fp, "\n");

    while (rd > 0)
    {
        rd = fread(buf_temp_8, sizeof(unsigned char), 4096, ifp);
        for (int i = 0; i < rd; i++)
        {
            buf_sets_8[(cnt++)%(NUM_SETS+1)] = buf_temp_8[i];
            if(buf_temp_8[i]==(unsigned char)(255))
            {
                fprintf(fp, "%d", ++epoch);
                for(int j=0; j<NUM_SETS; j++)
                {
                    fprintf(fp, ",%d", buf_sets_8[j]);
                }
                cnt = 0;
                fprintf(fp, "\n");
            }    
        }
    }

    printf("Epoch = %d \n", epoch);
    fclose (fp);
    fclose(ifp);
    return 0;
}