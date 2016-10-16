// stdafx.h: включаемый файл дл€ стандартных системных включаемых файлов
// или включаемых файлов дл€ конкретного проекта, которые часто используютс€, но
// не часто измен€ютс€
//
#define CURL_STATICLIB // используетс€ статическа€ сборка библиотеки

#ifdef _DEBUG
#pragma comment(lib,"libcurl_a_debug.lib")
#else
#pragma comment(lib,"libcurl_a.lib")
#endif

#pragma once

#include "targetver.h"

#define _USE_MATH_DEFINES 

#include <cmath>
#include <stdio.h>
//#include <tchar.h>
#include <iostream>
#include <string>
#include <time.h>
//#include <windows.h>
#include <ostream>
#include <fstream>
#include <vector>
#include <sys/stat.h>

// TODO: ”становите здесь ссылки на дополнительные заголовки, требующиес€ дл€ программы
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <boost/optional.hpp>
#include <curl/curl.h>