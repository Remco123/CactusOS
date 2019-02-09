int main()
{
	int call = 0;
    while(1)
    {
		for(int i = 0; i < 612000; i++)
				asm volatile("pause");
		
		int a = 0;
		__asm__ __volatile__("int $0x80" : "=a" (a) : "0" (call++));
    }
    return 1;
}
