#include <iostream>
#include "plocate.h"

int main()
{
	vector<string> fs,prunefs;
	read_plocate(fs,prunefs, "/etc/cruft/ignore");

	cout << '/' << endl;
	for (unsigned int i=0;i<fs.size();i++) {
		cout << fs[i] << endl;
	}
}
