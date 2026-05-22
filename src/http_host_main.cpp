#if defined(HTTP_BUILD_HOST_TEST_MAIN) && !defined(UNIT_TEST) && !defined(PIO_UNIT_TESTING)
int main()
{
    return 0;
}
#endif
