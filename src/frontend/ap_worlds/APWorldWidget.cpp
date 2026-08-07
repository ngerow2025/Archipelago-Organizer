//
// Created by birb on 7/6/26.
//

#include "APWorldWidget.h"

#include <QJsonObject>
#include <QMimeData>

#include <QDir>
#include <QFileInfo>

#include "libzippp.h"

#include <pybind11/embed.h> // everything needed for embedding
namespace py = pybind11;

using namespace libzippp;



// Iterates every .apworld entry in the zip without decompressing it.
// handler is a function that takes name, size, and readData as parameters.
// readData is itself a function you can call to decompress and read the
// entry (takes a callback of data + size); if handler never calls readData,
// that entry is never decompressed.
// handler returns true to keep iterating, false to stop early.
template <typename EntryHandler>
static bool forEachApworldEntry(const QString& zipFilePath, EntryHandler handler) {
    ZipArchive archive(zipFilePath.toStdString());

    if (!archive.open(ZipArchive::ReadOnly)) {
        qWarning() << "Failed to open zip file:" << zipFilePath;
        return false;
    }

    for (const ZipEntry& entry : archive.getEntries()) {
        if (!entry.isFile()) continue;

        QString name = QString::fromStdString(entry.getName());
        if (!name.endsWith(".apworld", Qt::CaseInsensitive)) continue;

        libzippp_uint64 entry_size = entry.getSize();

        // lazy accessor, only decompresses if the handler calls it
        auto readData = [&archive, &entry, entry_size](auto&& dataCallback) -> bool {
            return archive.readEntry(
                entry,
                [&dataCallback](const void* data, libzippp_uint64 size) -> bool {
                    //entry_size and size should be the same
                    dataCallback(data, size);
                    return true;
                },
                ZipArchive::Current, entry_size);
        };

        if (!handler(name, entry_size, readData)) {
            break; // handler asked us to stop early
        }
    }

    archive.close();
    return true;
}

// Process is a function that takes data, size, and name as parameters
template <typename Process>
static bool extractApworldFiles(const QString& zipFilePath, Process callback) {
    return forEachApworldEntry(zipFilePath, [&callback](const QString& name, libzippp_uint64 /*size*/, const auto& readData) {
        // always want the bytes here, so always call readData
        readData([&callback, &name](const void* data, libzippp_uint64 size) { callback(data, size, name); });
        return true;
    });
}

static bool zipContainsApworldFiles(const QString& zipFilePath) {
    bool found = false;
    // never touch readData, so no decompression happens; stop at first match
    forEachApworldEntry(zipFilePath, [&found](const QString& /*name*/, libzippp_uint64 /*size*/, const auto& /*readData*/) {
        found = true;
        return false;
    });
    return found;
}

APWorldWidget::APWorldWidget(QWidget* parent) : QWidget(parent) {
    setAcceptDrops(true);
}

void APWorldWidget::dropEvent(QDropEvent* event) {
    // accepts .apworld and .zip files with .apworlds in them and folders with .apworlds in them
    if (event->mimeData()->hasUrls()) {
        qDebug() << "Has urls: " << event->mimeData()->urls();
        QList<QUrl> urls = event->mimeData()->urls();
        for (const QUrl& url : urls) {
            if (!url.isLocalFile()) {
                qDebug() << "found non-local file: " << url.toString();
                continue;
            }
            QString localFile = url.toLocalFile();
            // check if dir or file
            QFileInfo fileInfo(localFile);
            if (fileInfo.isDir()) {
                qDebug() << "Found directory: " << localFile;
                // check for .apworld file
                QDir        dir(localFile);
                QStringList files = dir.entryList(QStringList() << "*.apworld", QDir::Files);
                for (const auto& file : files) {
                    installAPWorldFromDisk(dir.absoluteFilePath(file));
                }
            } else if (fileInfo.isFile()) {
                if (fileInfo.fileName().endsWith(".apworld")) {
                    installAPWorldFromDisk(localFile);
                } else if (fileInfo.fileName().endsWith(".zip")) {
                    extractApworldFiles(localFile, [this](const void* data, const uint64_t size, const QString& name) {
                        installAPWorldFromData(data, size, name);
                    });
                } else {
                    qWarning() << "Found non .apworld or .zip file: " << localFile;
                }
            } else {
                qWarning() << "Not a file or directory: " << localFile;
            }
        }
    } else {
        qWarning() << "Not a file with url: " << event->mimeData()->text();
    }
}

void APWorldWidget::dragEnterEvent(QDragEnterEvent* event) {
    // accepts .apworld and .zip files with .apworlds in them and folders with .apworlds in them
    if (event->mimeData()->hasUrls()) {
        qDebug() << "Has urls: " << event->mimeData()->urls();
        QList<QUrl> urls = event->mimeData()->urls();
        for (const QUrl& url : urls) {
            if (!url.isLocalFile()) {
                qDebug() << "found non-local file: " << url.toString();
                continue;
            }
            QString localFile = url.toLocalFile();
            // check if dir or file
            QFileInfo fileInfo(localFile);
            if (fileInfo.isDir()) {
                qDebug() << "Found directory: " << localFile;
                // check for .apworld file
                QDir        dir(localFile);
                QStringList files = dir.entryList(QStringList() << "*.apworld", QDir::Files);
                if (!files.isEmpty()) {
                    event->acceptProposedAction();
                    qDebug() << "Found .apworld file in directory: " << dir.absolutePath();
                    return;
                }
            } else if (fileInfo.isFile()) {
                if (fileInfo.fileName().endsWith(".apworld")) {
                    qDebug() << "Found file: " << localFile;
                    event->acceptProposedAction();
                } else if (fileInfo.fileName().endsWith(".zip")) {
                    qDebug() << "Found .zip file: " << localFile;
                    if (zipContainsApworldFiles(localFile)) {
                        event->acceptProposedAction();
                    }
                } else {
                    qWarning() << "Found non .apworld or .zip file: " << localFile;
                }
            } else {
                qWarning() << "Not a file or directory: " << localFile;
            }
        }
    } else {
        qWarning() << "Not a file with url: " << event->mimeData()->text();
    }
}

void APWorldWidget::installAPWorldFromDisk(const QString& filePath) {
    qDebug() << "installing .apworld file from disk: " << filePath;
    ZipArchive apWorldArchive(filePath.toStdString());
    installApWorld(&apWorldArchive, filePath);
}
void APWorldWidget::installAPWorldFromData(const void* data, uint64_t size, const QString& name) {
    // qDebug() << "installing .apworld file from data: " << name << " size: " << size;
    ZipArchive* apWorldArchive = ZipArchive::fromBuffer(data, size);
    installApWorld(apWorldArchive, name);
    ZipArchive::free(apWorldArchive);
    // Implement the logic to handle the .apworld data here
}

void getGameFromPython(const ZipArchive* archive, const QString& name) {
    py::scoped_interpreter guard{};
    py::module_::
}

void APWorldWidget::installApWorld(const ZipArchive* archive, const QString& name) {
    //get the first base folder in the zip and print it out along with the name
    std::string firstEntryPath = archive->getEntries().begin()->getName();
    size_t seporatorLocation = firstEntryPath.find_first_of('/');
    std::string firstBaseFolder = firstEntryPath.substr(0, seporatorLocation);
    // qDebug() << "First base folder in .apworld: " << firstBaseFolder << " name: " << name.toStdString().c_str();

    //look for the archipelago.json file
    ZipEntry manifest = archive->getEntry(firstBaseFolder + "/archipelago.json");
    if (manifest.isNull()) {
        qWarning() << "No archipelago.json file found in .apworld: " << name.toStdString().c_str();
        getGameFromPython(archive, name);
        return;
    }
    std::string manifestContents = manifest.readAsText();


    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(manifestContents.data(), &parseError);
    auto game = doc.object().constFind("game").value().toString();
    qDebug() << game;
}

