// TPM.cpp: определяет точку входа для консольного приложения.
//

#include "stdafx.h"
using namespace std;

int main()
{

	char x;

	boost::property_tree::ptree pt;
	boost::property_tree::ini_parser::read_ini("config.ini", pt);

	cout << pt.get<string>("Provider.name") << endl;
	cout << pt.get<string>("Provider.id") << endl;
	cout << pt.get<string>("Provider.key") << endl;
	cout << pt.get<string>("Output.type") << endl;
	cout << pt.get<string>("Output.dir") << endl;

	cin >> x;

	return 0;

}

