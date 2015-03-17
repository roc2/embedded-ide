#include "editorwidget.h"
#include "ui_editorwidget.h"
//#include "highlighter.h"

#include "qsvsh/qsvcolordef.h"
#include "qsvsh/qsvcolordeffactory.h"
#include "qsvsh/qsvlangdef.h"
#include "qsvsh/qsvsyntaxhighlighter.h"

#include <QDir>
#include <QSettings>
#include <QCompleter>
#include <QStringListModel>
#include <QMimeDatabase>

#include <QtDebug>

#include <QFileInfo>

EditorWidget::EditorWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::EditorWidget),
    defColors(0l),
    langDef(0l),
    syntax(0l)
{
    ui->setupUi(this);
    QAction *saveAction = new QAction(this);
    saveAction->setShortcut(QKeySequence("ctrl+s"));
    connect(saveAction, SIGNAL(triggered()), this, SLOT(save()));
    ui->editor->addAction(saveAction);
    QFont f(QSettings().value("editor/font/style", "DejaVu Sans Mono").toString(),
            QSettings().value("editor/font/size", 10).toInt());
    ui->editor->setFont(f);
    connect(ui->editor, SIGNAL(fileError(QString)), this, SIGNAL(editorError(QString)));
    connect(ui->editor, SIGNAL(modificationChanged(bool)), this, SIGNAL(modified(bool)));
}

EditorWidget::~EditorWidget()
{
    delete ui;
}

void EditorWidget::moveCursor(int row, int col)
{
    QTextDocument *doc = ui->editor->document();
    QTextBlock block = doc->begin();
    int off = 0;
    for(int y=1 ; y<row; y++ ) {
        off += block.length();
        if (block != doc->end())
            block = block.next();
        else
            return;
    }
    off+= col;

    QTextCursor c = ui->editor->textCursor();
    c.setPosition( off );
    ui->editor->setTextCursor( c );
    // ui->editor->grabKeyboard();
}

static QString findStyleByName(const QString& defaultName) {
    QDir d(":/qsvsh/qtsourceview/data/colors/");
    foreach(QString name, d.entryList(QStringList("*.xml"))) {
        QDomDocument doc("mydocument");
        QFile file(d.filePath(name));
        if (file.open(QIODevice::ReadOnly) && doc.setContent(&file)) {
            QDomNodeList itemDatas = doc.elementsByTagName("itemDatas");
            if (!itemDatas.isEmpty()) {
                QDomNamedNodeMap attr = itemDatas.at(0).attributes();
                QString name = attr.namedItem("name").toAttr().value();
                if (defaultName == name)
                    return file.fileName();
            }
        }
    }
    return QString();
}

bool EditorWidget::load(const QString &fileName)
{
    QFileInfo info(fileName);
    if (!ui->editor->load(info.absoluteFilePath()))
        return false;
    setWindowFilePath(info.absoluteFilePath());
    setWindowTitle(info.fileName());

    if (defColors)
        delete defColors;
    if (langDef)
        delete langDef;
    if (syntax)
        syntax->deleteLater();
    langDef = 0l;
    syntax = 0l;

    defColors = new QsvColorDefFactory( findStyleByName(QSettings().value("editor/colorstyle", "Kate").toString()) );
    QMimeDatabase db;
    QMimeType fType = db.mimeTypeForFile(info);
    if (fType.inherits("text/x-csrc")) {
        langDef   = new QsvLangDef( ":/qsvsh/qtsourceview/data/langs/c.lang" );
    } else if (fType.inherits("text/x-makefile")) {
        langDef   = new QsvLangDef( ":/qsvsh/qtsourceview/data/langs/makefile.lang" );
    }
    if (defColors && langDef) {
        syntax = new QsvSyntaxHighlighter( ui->editor->document() , defColors, langDef );
        syntax->setObjectName("syntaxer");
        QPalette p = ui->editor->palette();
        p.setColor(QPalette::Base, defColors->getColorDef("dsWidgetBackground").getBackground());
        ui->editor->setPalette(p);
        ui->editor->refreshHighlighterLines();
    }
    return true;
}

bool EditorWidget::save()
{
    qDebug() << "****** saving ******";
    return ui->editor->save();
}