#include <stdio.h>
#include <stdlib.h>

int main(){
	FILE *fp;  
	int **a;

	int size; 
	fp = fopen("pilot.txt", "r");
	fscanf(fp, "%d", &size);

	a = (int **)malloc((size+1) * sizeof(int *));

    for (int i=0; i<(size+1); i++)
         a[i] = (int *)malloc(2 * sizeof(int));

    int i = 0;
	int j = 0;


	int temp;
	while(fscanf(fp, "%d", &temp)!=EOF){  
	 	if(j%2==0){
	 		a[i][0] = temp;
	 	}
	 	else{
	 		a[i][1] = temp;
	 		i++;
	 	}
	 	j++;  
   	}
   



   	int dx, dy;
   	scanf("%d %d", &dx, &dy);

   	fp = fopen("pilot.txt", "w");

   	fprintf(fp, "%d\n", size);

   	for(int i=0; i<size; i++){
   		for(int j=0; j<2; j++){
   			if(j==0){
   				a[i][j] += dx;
   			}
   			else{
   				a[i][j] += dy;
   			}
   			fprintf(fp, "%d\n", a[i][j]);
   		}
   	}

   	fclose(fp);
}