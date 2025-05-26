#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>


int main() 
{
  FILE *f1 = fopen("t.txt","w");
  FILE *f2 = fopen("t.txt","w");

  for(char c = 'a'; c <= 'z'; c++)
  {
    if (c%2){
	    fprintf(f1, "%c", c);
    }
    else{
	    fprintf(f2, "%c", c);
    }
  }
  fclose(f1);
  fclose(f2);
  return 0;
}