#include <QString>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QTextCodec>

enum class EEncoding { 
    Utf8NoBOM,
    Utf8BOM,
    Ascii7,
    Ascii8
};

namespace {
QString q_toString(EEncoding encoding)
{
    switch (encoding) {
    case EEncoding::Ascii7:
        return "ASCII";
    case EEncoding::Ascii8:
        return "ASCII with codepage";
    case EEncoding::Utf8BOM:
        return "UTF-8 with BOM";
    case EEncoding::Utf8NoBOM:
        return "UTF-8 without BOM";
    }
    return "Unknown";
}
}

EEncoding detectEncoding(const QString& inputFileName);

int main(int argc, char* argv[])
{
    QTextStream qout(stdout);
    QTextStream qin(stdin);
    QStringList exts;
    exts << "*.ass";
    exts << "*.ssa";
    exts << "*.srt";
    exts << "*.txt";
    QDir dir; // current folder
    QStringList fileNames = dir.entryList(exts, QDir::Files);
    if (fileNames.isEmpty()) {
        qout << "No subtitle files detected." << endl;
        qout << "Press any key to continue . . ." << endl;
        qin.read(1);
        return 0;
    }
    // Backup
    qout << "Creating backup in `old` directory ";
    if (dir.mkdir("old"))
        qout << " [OK]" << endl;
    else
        qout << " [FAILED]" << endl;
    foreach (QString fileName, fileNames) {
        qout << "  Copying " << fileName << " to backup directory";
        if (QFile::copy(fileName, "old/" + fileName))
            qout << " [OK]" << endl;
        else
            qout << " [FAILED]" << endl;
    }
    qout << endl << "Converting files: " << endl;
    foreach (QString fileName, fileNames) {
        qout << "  Converting file " << fileName;
        EEncoding detectedEncoding = detectEncoding(fileName);
        qout << ", encoding: " << q_toString(detectedEncoding);
        if (detectedEncoding == EEncoding::Utf8BOM) {
            qout << " [SKIP]" << endl;
            continue;
        }
        QFile file(fileName);
        QTextCodec* codec = QTextCodec::codecForName("UTF-8");
        QString buf;
        if (file.open(QFile::ReadOnly | QIODevice::Text)) {
            QTextStream input(&file);
            if (detectedEncoding == EEncoding::Utf8NoBOM)
                input.setCodec(codec);
            buf = input.readAll();
            file.close();
        }
        if (file.open(QFile::WriteOnly | QFile::Truncate | QFile::Text)) {
            QTextStream output(&file);
            output.setCodec(codec);
            output.setGenerateByteOrderMark(true);
            output << buf;
            file.close();
        }
        qout << " [OK]" << endl;
    }
    qout << "Press any key to continue . . ." << endl;
    qin.read(1);
}

// Shamelessly copied from Notepad++ sources
EEncoding utf8_7bits_8bits(quint8* buf, int len)
{
    int rv = 1;
    int ascii7only = 1;
    quint8* sx = buf;
    quint8* endx = sx + len;
    while (sx < endx) {
        if (!(*sx)) {
            ascii7only = 0;
            rv = 0;
            break;
        } else if ((*sx) < 0x80) {
            sx++;
        } else if ((*sx) < (0x80 + 0x40)) {
            ascii7only = 0;
            rv = 0;
            break;
        } else if ((*sx) < (0x80 + 0x40 + 0x20)) {
            ascii7only = 0;
            if (sx >= endx - 1)
                break;
            if (!((*sx) & 0x1F) || (sx[1] & (0x80 + 0x40)) != 0x80) {
                rv = 0;
                break;
            }
            sx += 2;
        } else if ((*sx) < (0x80 + 0x40 + 0x20 + 0x10)) {
            ascii7only = 0;
            if (sx >= endx - 2)
                break;
            if (!((*sx) & 0xF) || (sx[1] & 0x80 + 0x40) != 0x80 ||
                (sx[2] & 0x80 + 0x40) != 0x80) {
                rv = 0;
                break;
            }
            sx += 3;
        } else {
            ascii7only = 0;
            rv = 0;
            break;
        }
    }
    if (ascii7only)
        return EEncoding::Ascii7;
    if (rv)
        return EEncoding::Utf8NoBOM;
    return EEncoding::Ascii8;
}

EEncoding detectEncoding(const QString& inputFileName)
{
    QFile infile(inputFileName);
    if (infile.open(QFile::ReadOnly | QIODevice::Text)) {
        QByteArray data = infile.readAll();
        int len = data.count();
        const char* cbuf = data.data();
        quint8* buf = (quint8*)cbuf;
        if (len > 2 && buf[0] == 0xEF && buf[1] == 0xBB && buf[2] == 0xBF) {
            return EEncoding::Utf8BOM;
        } else {
            return utf8_7bits_8bits(buf, len);
        }
    }
    return EEncoding::Ascii7;
}
