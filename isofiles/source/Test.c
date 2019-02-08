int main()
{
    /*
    while(1)
    {
        asm volatile ("int $0x80");

        for(int i = 0; i < 1200000; i++)
        {
            asm volatile("pause");
        }
    }
    */
    while(1)
    {
            asm volatile("pause");
    }
    return 1;
}
