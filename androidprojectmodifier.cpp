#include "androidprojectmodifier.h"

AndroidProjectModifier::AndroidProjectModifier(QString path)
{
    m_directory = QDir(path);
}

void AndroidProjectModifier::addSupportLibrary()
{
    const auto path = QStringLiteral("/app/build.gradle");

    auto fileContent = getFileContent(path);
    fileContent = fileContent.replace(
                "implementation 'com.github.GoogleChrome.custom-tabs-client:customtabs:91b4a1270b'",
                "implementation 'com.github.GoogleChrome.custom-tabs-client:customtabs:91b4a1270b'\n\timplementation 'com.android.support:appcompat-v7:28.0.0'"
    );

    updateFileContent(path, fileContent);
}

void AndroidProjectModifier::setBasicData(QHash<QString, QString> data)
{
    const auto path = QStringLiteral("/app/build.gradle");
    auto fileContent = getFileContent(path);

    fileContent = fileContent.replace("org.chromium.twa.svgomg", data.value("package"))
            .replace("svgomg.firebaseapp.com", data.value("hostname"))
            .replace("launchUrl: '/'", "launchUrl: '" + data.value("start_url") + "'")
            .replace("SVGOMG TWA", data.value("short_name"))
            .replace("#303F9F", data.value("theme_color"))
            .replace("#bababa", data.value("background_color"))
            .replace("versionCode 3", "versionCode 1")
            .replace("versionName \"1.1.1\"", "versionName \"1.0.0\"");

    updateFileContent(path, fileContent);
}

void AndroidProjectModifier::addImages(QList<QHash<QString, QString>> images)
{
    QList<QHash<QString, QString>>::const_iterator iterator;

    std::sort(images.begin(), images.end(), [](const auto &item1, const auto &item2) {
        if (item1.value("size").toInt() == item2.value("size").toInt()) {
            return 0;
        }

        return item1.value("size").toInt() > item2.value("size").toInt() ? 1 : -1;
    });

    auto largestImage = images.at(0);
    QFile imageFile(downloadImage(largestImage.value("url")));
    largestImage.insert("path", imageFile.fileName());

    QFile mdpi(resizeImage(largestImage, "48"));
    QFile hdpi(resizeImage(largestImage, "72"));
    QFile xhdpi(resizeImage(largestImage, "96"));
    QFile xxhdpi(resizeImage(largestImage, "144"));
    QFile xxxhdpi(resizeImage(largestImage, "192"));

    const auto movePath = m_directory.absolutePath() + "/app/src/main/res/mipmap-";

    QFile::remove(movePath + "mdpi" + "/ic_launcher.png") && mdpi.rename(movePath + "mdpi" + "/ic_launcher.png");
    QFile::remove(movePath + "hdpi" + "/ic_launcher.png") && hdpi.rename(movePath + "hdpi" + "/ic_launcher.png");
    QFile::remove(movePath + "xhdpi" + "/ic_launcher.png") && xhdpi.rename(movePath + "xhdpi" + "/ic_launcher.png");
    QFile::remove(movePath + "xxhdpi" + "/ic_launcher.png") && xxhdpi.rename(movePath + "xxhdpi" + "/ic_launcher.png");
    QFile::remove(movePath + "xxxhdpi" + "/ic_launcher.png") && xxxhdpi.rename(movePath + "xxxhdpi" + "/ic_launcher.png");

    imageFile.remove();
}

QString AndroidProjectModifier::getFileContent(const QString filepath)
{
    QFile file(m_directory.absolutePath() + filepath);
    if(!file.exists() || !file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        throw QString("The file does not exist or could not be opened for reading");
    }

    QTextStream stream(&file);

    QString result = stream.readAll();

    file.close();

    return result;
}

void AndroidProjectModifier::updateFileContent(const QString filepath, const QString content)
{
    QFile file(m_directory.absolutePath() + filepath);
    if(!file.exists()  || !file.open(QIODevice::ReadWrite | QIODevice::Text | QIODevice::Truncate)) {
        throw QString("The file does not exist or could not be opened for writing");
    }

    QTextStream stream(&file);
    stream << content;

    file.close();
}

QString AndroidProjectModifier::downloadImage(const QString url)
{
    QNetworkAccessManager manager;
    QNetworkRequest request((QUrl(url)));
    QEventLoop loop;

    QObject::connect(&manager, &QNetworkAccessManager::finished, &loop, &QEventLoop::quit);

    auto reply = manager.get(request);
    loop.exec();

    auto result = reply->readAll();
    delete reply;

    QString tmpDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);

    QFile tmpFile;

    do {
        tmpFile.setFileName(tmpDir + "/" + randomString());
    } while (tmpFile.exists());

    if(!tmpFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        throw QString("Could not open temporary file for writing");
    }

    tmpFile.write(result);
    tmpFile.close();

    return tmpFile.fileName();
}

QString AndroidProjectModifier::randomString(int length)
{
    const auto possibleCharacters = QStringLiteral("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789");

    QString randomString;
    for(int i = 0; i < length; ++i)
    {
       int index = qrand() % possibleCharacters.length();
       QChar nextChar = possibleCharacters.at(index);
       randomString.append(nextChar);
    }
    return randomString;
}

QString AndroidProjectModifier::resizeImage(const QHash<QString, QString> imageData, QString size)
{
    QString tmpDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);

    QFile tmpFile;

    do {
        tmpFile.setFileName(tmpDir + "/" + randomString());
    } while (tmpFile.exists());

    QProcess convert;
    convert.start("convert", QStringList() << imageData.value("path") << "-resize" << size + "x" + size << tmpFile.fileName());
    convert.waitForFinished();

    return tmpFile.fileName();
}
