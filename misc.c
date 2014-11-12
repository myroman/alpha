void rmnl(char* s) {
	int ln = strlen(s) - 1;
	if (s[ln] == '\n')
	    s[ln] = '\0';
	printf("%c\n",s[ln]);	
}

char* createTmplFilename(){
	char*s = malloc(10);
/*
	s[0]='9';
	s[1]='5';
	s[2]='2';
	s[3]='4';
	s[4]='5';
	s[5]='7';
*/
	s[0]='r';
	s[1]='O';
	s[2]='m';
	s[3]='A';
	s[4]='X';
	s[5]='X';
	s[6]='X';
	s[7]='X';
	s[8]='X';
	s[9]='X';
	return s;
}