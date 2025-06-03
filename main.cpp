#include <QApplication>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QFileDialog>
#include "shapeViewer.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QMainWindow window;
    QWidget *central = new QWidget();
    QHBoxLayout *layout = new QHBoxLayout(central);

    // Viewer
    ShapeViewer *viewer = new ShapeViewer();
    viewer->setMinimumSize(500, 500);

    // Controls
    QWidget *controlPanel = new QWidget();
    QVBoxLayout *controls = new QVBoxLayout(controlPanel);

    auto makeControl = [&](const QString &label, int min, int max, int value, auto slot)
    {
        QHBoxLayout *row = new QHBoxLayout();
        row->addWidget(new QLabel(label));
        QSpinBox *spin = new QSpinBox();
        spin->setRange(min, max);
        spin->setValue(value);
        QObject::connect(spin, SIGNAL(valueChanged(int)), viewer, slot);
        row->addWidget(spin);
        controls->addLayout(row);
    };

    makeControl("Radius:", 5, 200, 50, SLOT(setBaseRadius(int)));
    makeControl("Height:", 5, 400, 100, SLOT(setHeight(int)));
    makeControl("Subdivs:", 3, 100, 20, SLOT(setSubdivisions(int)));

    // Button: Load Texture
    QPushButton *loadTextureButton = new QPushButton("Load Texture");
    QObject::connect(loadTextureButton, &QPushButton::clicked, [&]()
                     {
        QString path = QFileDialog::getOpenFileName(nullptr, "Select Texture Image", "", "Images (*.png *.jpg *.bmp)");
        if (!path.isEmpty()) {
            viewer->loadTexture(path);
        } });
    controls->addWidget(loadTextureButton);

    QPushButton *meshButton = new QPushButton("Wireframe");
    QPushButton *colorButton = new QPushButton("Colored");
    QPushButton *textureButton = new QPushButton("Textured");

    controls->addWidget(meshButton);
    controls->addWidget(colorButton);
    controls->addWidget(textureButton);

    QObject::connect(meshButton, &QPushButton::clicked, [&]()
                     { viewer->setDisplayMode(DisplayMode::Wireframe); });
    QObject::connect(colorButton, &QPushButton::clicked, [&]()
                     { viewer->setDisplayMode(DisplayMode::Colored); });
    QObject::connect(textureButton, &QPushButton::clicked, [&]()
                     { viewer->setDisplayMode(DisplayMode::Textured); });

    layout->addWidget(viewer, 1);
    layout->addWidget(controlPanel);

    window.setCentralWidget(central);
    window.resize(800, 600);
    window.show();

    return app.exec();
}
