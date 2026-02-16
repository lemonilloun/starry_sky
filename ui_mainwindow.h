/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.9.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <MyGLWidget.h>
#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralwidget;
    QLabel *ra_label;
    QLabel *label;
    QDoubleSpinBox *observerRaSpinBox;
    QDoubleSpinBox *observerDecSpinBox;
    QLabel *label_2;
    QLabel *label_3;
    QLabel *label_4;
    QDoubleSpinBox *maxMagnitudeSpinBox;
    QDoubleSpinBox *fovYSpinBox;
    QLabel *ra_label_2;
    QLabel *label_6;
    QDoubleSpinBox *fovXSpinBox;
    QLabel *label_7;
    QFrame *line_2;
    QPushButton *buildMapButton;
    QDoubleSpinBox *beta2SpinBox;
    QLabel *phi_label;
    QDoubleSpinBox *pSpinBox;
    QLabel *psi_label;
    QLabel *theta_label;
    QDoubleSpinBox *beta1SpinBox;
    QLabel *label_9;
    QFrame *line_3;
    QFrame *line_4;
    QFrame *line_5;
    QWidget *MapWidget;
    MyGLWidget *GLWidget;
    QFrame *line_6;
    QPushButton *settingsButton;
    QSpinBox *DaySpinBox;
    QLabel *label_5;
    QSpinBox *MonthSpinBox;
    QLabel *label_8;
    QSpinBox *YearSpinBox;
    QLabel *label_10;
    QLabel *label_11;
    QMenuBar *menubar;
    QMenu *menustarry_sky;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(1368, 810);
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName("centralwidget");
        ra_label = new QLabel(centralwidget);
        ra_label->setObjectName("ra_label");
        ra_label->setGeometry(QRect(1100, 70, 161, 20));
        label = new QLabel(centralwidget);
        label->setObjectName("label");
        label->setGeometry(QRect(1090, 100, 151, 20));
        observerRaSpinBox = new QDoubleSpinBox(centralwidget);
        observerRaSpinBox->setObjectName("observerRaSpinBox");
        observerRaSpinBox->setGeometry(QRect(1240, 71, 101, 21));
        observerRaSpinBox->setDecimals(6);
        observerRaSpinBox->setMinimum(-1000.000000000000000);
        observerRaSpinBox->setMaximum(1000.000000000000000);
        observerDecSpinBox = new QDoubleSpinBox(centralwidget);
        observerDecSpinBox->setObjectName("observerDecSpinBox");
        observerDecSpinBox->setGeometry(QRect(1240, 100, 101, 21));
        observerDecSpinBox->setDecimals(6);
        observerDecSpinBox->setMinimum(-1000.000000000000000);
        observerDecSpinBox->setMaximum(1000.000000000000000);
        label_2 = new QLabel(centralwidget);
        label_2->setObjectName("label_2");
        label_2->setGeometry(QRect(1110, 40, 231, 21));
        label_3 = new QLabel(centralwidget);
        label_3->setObjectName("label_3");
        label_3->setGeometry(QRect(1140, 140, 231, 20));
        label_4 = new QLabel(centralwidget);
        label_4->setObjectName("label_4");
        label_4->setGeometry(QRect(1090, 360, 331, 61));
        maxMagnitudeSpinBox = new QDoubleSpinBox(centralwidget);
        maxMagnitudeSpinBox->setObjectName("maxMagnitudeSpinBox");
        maxMagnitudeSpinBox->setGeometry(QRect(1290, 390, 61, 22));
        maxMagnitudeSpinBox->setMaximum(10.000000000000000);
        fovYSpinBox = new QDoubleSpinBox(centralwidget);
        fovYSpinBox->setObjectName("fovYSpinBox");
        fovYSpinBox->setGeometry(QRect(1270, 200, 71, 22));
        fovYSpinBox->setMaximum(180.000000000000000);
        ra_label_2 = new QLabel(centralwidget);
        ra_label_2->setObjectName("ra_label_2");
        ra_label_2->setGeometry(QRect(1090, 170, 171, 20));
        label_6 = new QLabel(centralwidget);
        label_6->setObjectName("label_6");
        label_6->setGeometry(QRect(1090, 200, 171, 20));
        fovXSpinBox = new QDoubleSpinBox(centralwidget);
        fovXSpinBox->setObjectName("fovXSpinBox");
        fovXSpinBox->setGeometry(QRect(1270, 170, 71, 22));
        fovXSpinBox->setMaximum(180.000000000000000);
        label_7 = new QLabel(centralwidget);
        label_7->setObjectName("label_7");
        label_7->setGeometry(QRect(1110, 420, 301, 31));
        line_2 = new QFrame(centralwidget);
        line_2->setObjectName("line_2");
        line_2->setGeometry(QRect(1080, 120, 281, 20));
        line_2->setFrameShape(QFrame::Shape::HLine);
        line_2->setFrameShadow(QFrame::Shadow::Sunken);
        buildMapButton = new QPushButton(centralwidget);
        buildMapButton->setObjectName("buildMapButton");
        buildMapButton->setGeometry(QRect(1170, 730, 121, 32));
        beta2SpinBox = new QDoubleSpinBox(centralwidget);
        beta2SpinBox->setObjectName("beta2SpinBox");
        beta2SpinBox->setGeometry(QRect(1260, 300, 61, 22));
        beta2SpinBox->setDecimals(2);
        beta2SpinBox->setMinimum(-180.000000000000000);
        beta2SpinBox->setMaximum(180.000000000000000);
        phi_label = new QLabel(centralwidget);
        phi_label->setObjectName("phi_label");
        phi_label->setGeometry(QRect(1110, 326, 131, 20));
        pSpinBox = new QDoubleSpinBox(centralwidget);
        pSpinBox->setObjectName("pSpinBox");
        pSpinBox->setGeometry(QRect(1260, 330, 61, 22));
        pSpinBox->setDecimals(2);
        pSpinBox->setMinimum(-180.000000000000000);
        pSpinBox->setMaximum(180.000000000000000);
        psi_label = new QLabel(centralwidget);
        psi_label->setObjectName("psi_label");
        psi_label->setGeometry(QRect(1100, 300, 141, 20));
        theta_label = new QLabel(centralwidget);
        theta_label->setObjectName("theta_label");
        theta_label->setGeometry(QRect(1100, 270, 141, 20));
        beta1SpinBox = new QDoubleSpinBox(centralwidget);
        beta1SpinBox->setObjectName("beta1SpinBox");
        beta1SpinBox->setGeometry(QRect(1260, 271, 61, 21));
        beta1SpinBox->setDecimals(2);
        beta1SpinBox->setMinimum(-180.000000000000000);
        beta1SpinBox->setMaximum(180.000000000000000);
        label_9 = new QLabel(centralwidget);
        label_9->setObjectName("label_9");
        label_9->setGeometry(QRect(1140, 240, 191, 20));
        line_3 = new QFrame(centralwidget);
        line_3->setObjectName("line_3");
        line_3->setGeometry(QRect(1080, 220, 281, 20));
        line_3->setFrameShape(QFrame::Shape::HLine);
        line_3->setFrameShadow(QFrame::Shadow::Sunken);
        line_4 = new QFrame(centralwidget);
        line_4->setObjectName("line_4");
        line_4->setGeometry(QRect(1080, 350, 281, 20));
        line_4->setFrameShape(QFrame::Shape::HLine);
        line_4->setFrameShadow(QFrame::Shadow::Sunken);
        line_5 = new QFrame(centralwidget);
        line_5->setObjectName("line_5");
        line_5->setGeometry(QRect(1080, 410, 281, 20));
        line_5->setFrameShape(QFrame::Shape::HLine);
        line_5->setFrameShadow(QFrame::Shadow::Sunken);
        MapWidget = new QWidget(centralwidget);
        MapWidget->setObjectName("MapWidget");
        MapWidget->setGeometry(QRect(-1, 4, 1081, 761));
        MapWidget->setMinimumSize(QSize(1081, 761));
        MapWidget->setMaximumSize(QSize(1081, 761));
        MapWidget->setInputMethodHints(Qt::InputMethodHint::ImhDigitsOnly);
        GLWidget = new MyGLWidget(centralwidget);
        GLWidget->setObjectName("GLWidget");
        GLWidget->setGeometry(QRect(1090, 520, 271, 200));
        line_6 = new QFrame(centralwidget);
        line_6->setObjectName("line_6");
        line_6->setGeometry(QRect(1080, 20, 281, 20));
        line_6->setFrameShape(QFrame::Shape::HLine);
        line_6->setFrameShadow(QFrame::Shadow::Sunken);
        settingsButton = new QPushButton(centralwidget);
        settingsButton->setObjectName("settingsButton");
        settingsButton->setGeometry(QRect(1130, 0, 191, 32));
        DaySpinBox = new QSpinBox(centralwidget);
        DaySpinBox->setObjectName("DaySpinBox");
        DaySpinBox->setGeometry(QRect(1140, 480, 42, 22));
        DaySpinBox->setMaximum(31);
        label_5 = new QLabel(centralwidget);
        label_5->setObjectName("label_5");
        label_5->setGeometry(QRect(1120, 480, 16, 20));
        MonthSpinBox = new QSpinBox(centralwidget);
        MonthSpinBox->setObjectName("MonthSpinBox");
        MonthSpinBox->setGeometry(QRect(1210, 480, 42, 22));
        MonthSpinBox->setMinimum(1);
        MonthSpinBox->setMaximum(12);
        label_8 = new QLabel(centralwidget);
        label_8->setObjectName("label_8");
        label_8->setGeometry(QRect(1190, 480, 20, 20));
        YearSpinBox = new QSpinBox(centralwidget);
        YearSpinBox->setObjectName("YearSpinBox");
        YearSpinBox->setGeometry(QRect(1270, 480, 61, 22));
        YearSpinBox->setMinimum(1301);
        YearSpinBox->setMaximum(2250);
        label_10 = new QLabel(centralwidget);
        label_10->setObjectName("label_10");
        label_10->setGeometry(QRect(1260, 480, 20, 20));
        label_11 = new QLabel(centralwidget);
        label_11->setObjectName("label_11");
        label_11->setGeometry(QRect(1140, 450, 201, 21));
        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName("menubar");
        menubar->setGeometry(QRect(0, 0, 1368, 24));
        menustarry_sky = new QMenu(menubar);
        menustarry_sky->setObjectName("menustarry_sky");
        MainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName("statusbar");
        MainWindow->setStatusBar(statusbar);

        menubar->addAction(menustarry_sky->menuAction());

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "MainWindow", nullptr));
        ra_label->setText(QCoreApplication::translate("MainWindow", "RA (\321\200\320\260\320\264\320\270\320\260\320\275\321\213)", nullptr));
        label->setText(QCoreApplication::translate("MainWindow", " Dec (\321\200\320\260\320\264\320\270\320\260\320\275\321\213)", nullptr));
        label_2->setText(QCoreApplication::translate("MainWindow", "\320\237\320\260\321\200\320\260\320\274\320\265\321\202\321\200\321\213 \320\275\320\260\320\261\320\273\321\216\320\264\320\260\321\202\320\265\320\273\321\217", nullptr));
        label_3->setText(QCoreApplication::translate("MainWindow", "\320\237\320\260\321\200\320\260\320\274\320\265\321\202\321\200\321\213 \320\272\320\260\320\274\320\265\321\200\321\213", nullptr));
        label_4->setText(QCoreApplication::translate("MainWindow", "\320\234\320\260\320\272\321\201\320\270\320\274\320\274\320\260\320\273\321\214\320\275\320\276\320\275 \320\267\320\275\320\260\321\207\320\265\320\275\320\270\320\265\n"
"\320\267\320\262\320\265\320\267\320\264\320\275\320\276\320\271 \320\262\320\265\320\273\320\270\321\207\320\270\320\275\321\213", nullptr));
        ra_label_2->setText(QCoreApplication::translate("MainWindow", "\320\236\320\261\320\273\320\260\321\201\321\202\321\214 X (\320\263\321\200\320\260\320\264\321\203\321\201\321\213)", nullptr));
        label_6->setText(QCoreApplication::translate("MainWindow", "\320\236\320\261\320\273\320\260\321\201\321\202\321\214 Y (\320\263\321\200\320\260\320\264\321\203\321\201\321\213)", nullptr));
        label_7->setText(QCoreApplication::translate("MainWindow", "\320\227\320\262\321\221\320\267\320\264\320\275\321\213\320\271 \320\272\320\260\321\202\320\260\320\273\320\276\320\263: Hipparcos", nullptr));
        buildMapButton->setText(QCoreApplication::translate("MainWindow", "\320\237\320\276\321\201\321\202\321\200\320\276\320\270\321\202\321\214", nullptr));
        phi_label->setText(QCoreApplication::translate("MainWindow", "\320\243\320\263\320\276\320\273 p (\320\263\321\200\320\260\320\264\321\203\321\201\321\213)", nullptr));
        psi_label->setText(QCoreApplication::translate("MainWindow", "\320\243\320\263\320\276\320\273 \316\2622 (\320\263\321\200\320\260\320\264\321\203\321\201\321\213)", nullptr));
        theta_label->setText(QCoreApplication::translate("MainWindow", "\320\243\320\263\320\276\320\273 \316\2621 (\320\263\321\200\320\260\320\264\321\203\321\201\321\213)", nullptr));
        label_9->setText(QCoreApplication::translate("MainWindow", "\320\237\320\260\321\200\320\260\320\274\320\265\321\202\321\200\321\213 \320\277\320\276\320\262\320\276\321\200\320\276\321\202\320\260 ", nullptr));
        settingsButton->setText(QCoreApplication::translate("MainWindow", "\320\224\320\276\320\277 \320\235\320\260\321\201\321\202\321\200\320\276\320\271\320\272\320\270", nullptr));
        label_5->setText(QCoreApplication::translate("MainWindow", "\320\224", nullptr));
        label_8->setText(QCoreApplication::translate("MainWindow", "M", nullptr));
        label_10->setText(QCoreApplication::translate("MainWindow", "\320\223", nullptr));
        label_11->setText(QCoreApplication::translate("MainWindow", "\320\222\321\200\320\265\320\274\321\217 \320\275\320\260\320\261\320\273\321\216\320\264\320\265\320\275\320\270\321\217", nullptr));
        menustarry_sky->setTitle(QCoreApplication::translate("MainWindow", "\320\230\320\274\320\274\320\270\321\202\320\260\321\202\320\276\321\200 \320\267\320\262\320\265\320\267\320\264\320\275\320\276\320\263\320\276 \320\275\320\265\320\261\320\260", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
