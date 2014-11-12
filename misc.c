void rmnl(char* s) {
	int ln = strlen(s) - 1;
	if (s[ln] == '\n')
	    s[ln] = '\0';
	printf("%c\n",s[ln]);	
}

char* getUnique6(){
	char*s = malloc(6);
	s[0]='9';
	s[1]='5';
	s[2]='2';
	s[3]='4';
	s[4]='5';
	s[5]='7';
	return s;
}