# getoptW

A completely wchar_t port of MinGW's port of getopt.c/h.  Implements getoptW, getoptW_long, getoptW_long_only.

I really just went through all the code in the MinGW repo and replaced all instances of char with wchar_t.  This should work and it's free.  The other option on the web is GPL which means you can't use it commercially unless you compile it as a DLL.  This one is very small and light.

Here's the licence preamble from the MinGW version.  By using this you agree to all the same terms as well as the Selection of Venue paragraph.

##Licence

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice, this permission notice, and the following
disclaimer shall be included in all copies or substantial portions of
the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.


### Selection of Venue:

This licence is governed by the laws of Ontario, Canada and any dispute 
shall be finally resolved by the courts in London, Ontario, Canada.


## Usage
You can pretty much just follow the getopt, getopt_long, getopt_long_only docs.
```c
    struct option long_options[] =
            {
                    {L"range", required_argument, 0, L'r'},
                    {L"username", required_argument, 0, L'u'},
                    {L"domain", required_argument, 0, L'd'},
                    {L"password", required_argument, 0, L'p'},
                    {L"loglevel", required_argument, 0, L'l'},
                    {L"hostfile", required_argument, 0, L'h'},
                    {L"threadmax", required_argument, 0, L't'},
                    {0, 0, 0, 0}
            };

    int option;


    int option_index = 0;
    int loglevel = 0;
    bool hostrange_defined = false;
    bool hostfile_defined = false;
    int threadmax = 0;

    while(1) {
        option = getoptW_long(argc, argv, L"h:u:d:p:l:r:", long_options, &option_index);

        if(option == -1) break;

        DUSTERR err;

        switch (option)
        {
            case L't':
                threadmax = _wtoi(optarg);
                if (threadmax > 100)
                {
                    display_error("Error: Too many threads specified.  Max is 100.");
                    return DUST_ERR_THREAD_MAX_EXCEEDED;
                }
                if(threadmax < 1)
                {
                    display_error("Error: Too few threads specified.  Min is 1.");
                    return DUST_ERR_THREAD_MIN_EXCEEDED;
                }

                settings->max_threads = (unsigned int)threadmax;

                break;
            case L'h':
                if(hostrange_defined)
                {
                    display_error("Error: Both a host range and host file were defined.  Please define only one.");
                    return DUST_ERR_HOSTFILE_AND_HOSTRANGE_BOTH_DEFINED;
                }
                settings->hostfile = wcsdup(optarg);
                err = read_hosts_to_array(settings->hostfile, &settings->hosts);

                if(err != DUST_ERR_SUCCESS) {
                    return err;
                }

                hostfile_defined = true;
                break;

            case L'l':
                loglevel = _wtoi(optarg);
                if(loglevel < 1 || loglevel > 3)
                {
                    display_error("Invalid log level");
                    return DUST_ERR_INVALID_PARAMETER_VALUE;
                }
                settings->loglevel = loglevel;
                break;

            case L'u':
                settings->username = wcsdup(optarg);
                break;

            case L'p':
                settings->password = wcsdup(optarg);
                break;

            case L'd':
                settings->domain = wcsdup(optarg);
                break;

            case L'r':
                if(hostfile_defined)
                {
                    display_error("Error: Both a host range and host file were defined.  Please define only one.");
                    return DUST_ERR_HOSTFILE_AND_HOSTRANGE_BOTH_DEFINED;
                }
                ConvertTCharToUtf8(optarg, &settings->hostrange);
                err = parse_host_range(settings->hostrange, &settings->hosts);
                if (DUST_ERR_SUCCESS != err)
                {
                    display_error("Error: Invalid host range.");
                    return err;
                }
                hostrange_defined = true;
                break;
        }
    }

```