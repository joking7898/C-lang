void main()
{
	/** test documentation comment.*/
	int testString[10] = "dk qorhvmek";	//string constant를 테스트한다.
	write("\101");						//escape sequence 테스트
	write("\ttap\ttap\t\n");				//escape sequence 테스트
	write("\x41 : A'll be printed");			//escape sequence 테스트	
	
	double testDouble = .3e+7;			//실수연산 테스트
	testDouble = 13.e-1;
	testDouble = 13.024;
	testDouble = 13.024e-2;
	testDouble = 13.024e+2;
	for();					//for 테스트
	switch (testDouble);	//switch 테스트
	case 1: break;			//case, ':', break 테스트
	goto;					//goto 테스트
	continue;				//continue 테스트
	return ;
}
