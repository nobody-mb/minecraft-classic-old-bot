#include "main.h"

int main (int argc, const char * argv[])
{


	return mc_proxy_startup("fatpig", "ip", 
				9001, "mppass");
}