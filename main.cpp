//#include "mainwindow.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
//#include <QApplication>

using namespace std;

int main(int argc, char *argv[])
{

    ifstream iFile;
    string line1 = "", line2 = "", first = "", sec = "";
    int location = 0;
    iFile.open("Catalogue.csv");
    getline(iFile, line1);
    getline(iFile, line2);
    
    location = line2.find(',');
    first = line2.substr(0,location);
    sec = line2.substr(location + 1, line2.length());
    cout << first << sec << endl;


    // Инициализация приложения Qt
    //QApplication a(argc, argv);
    //MainWindow w;
    //w.show();
    return 0;
}
