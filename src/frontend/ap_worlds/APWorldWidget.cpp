//
// Created by birb on 7/6/26.
//
#include <pybind11/embed.h>  // everything needed for embedding
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
// must be included first because it potentially interfears with standard marcros

#include <QDir>
#include <QFileInfo>
#include <QJsonObject>
#include <QMimeData>

#include "APWorldWidget.h"
#include "libzippp.h"

namespace py = pybind11;
using namespace pybind11::literals;

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
                    // entry_size and size should be the same
                    dataCallback(data, size);
                    return true;
                },
                ZipArchive::Current, entry_size);
        };

        if (!handler(name, entry_size, readData)) {
            break;  // handler asked us to stop early
        }
    }

    archive.close();
    return true;
}

// Process is a function that takes data, size, and name as parameters
template <typename Process>
static bool extractApworldFiles(const QString& zipFilePath, Process callback) {
    return forEachApworldEntry(
        zipFilePath,
        [&callback](const QString& name, libzippp_uint64 /*size*/, const auto& readData) {
            // always want the bytes here, so always call readData
            readData([&callback, &name](const void* data, libzippp_uint64 size) {
                callback(data, size, name);
            });
            return true;
        });
}

static bool zipContainsApworldFiles(const QString& zipFilePath) {
    bool found = false;
    // never touch readData, so no decompression happens; stop at first match
    forEachApworldEntry(zipFilePath, [&found](const QString& /*name*/, libzippp_uint64 /*size*/,
                                              const auto& /*readData*/) {
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
                    extractApworldFiles(localFile, [this](const void* data, const uint64_t size,
                                                          const QString& name) {
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
    apWorldArchive.open();
    installApWorld(&apWorldArchive, filePath);
}
void APWorldWidget::installAPWorldFromData(const void* data, uint64_t size, const QString& name) {
    // qDebug() << "installing .apworld file from data: " << name << " size: " << size;
    ZipArchive* apWorldArchive = ZipArchive::fromBuffer(data, size);
    installApWorld(apWorldArchive, name);
    ZipArchive::free(apWorldArchive);
    // Implement the logic to handle the .apworld data here
}

class TraversalNode {
public:
    TraversalNode(const ZipArchive& archive, const std::string& path) : archive(archive), path(path) {
    }

    std::vector<TraversalNode> iterdir() {
        qDebug() << "Iterating directory: " << path.c_str();
        std::vector<TraversalNode> result{};
        for(auto& entry : archive.getEntries()) {
            std::string entryName = entry.getName();
            if (entryName == path) {
                continue;
            }
            if (entryName.starts_with(path)) {
                unsigned int depth = std::count(entryName.begin(), entryName.end(), '/');
                unsigned int pathDepth = std::count(path.begin(), path.end(), '/');
                if (depth == pathDepth + 1 && path.back() == '/') {
                    result.push_back(TraversalNode(archive, entryName));
                }else if (depth == pathDepth && path.back() != '/') {
                    result.push_back(TraversalNode(archive, entryName));
                }
                
            }
        }
        return result;
    }

    py::object is_dir() {
        qDebug() << "Checking if directory: " << path.c_str();
        assert(archive.isOpen());
        assert(!archive.getEntry(path).isNull());
        return archive.getEntry(path).isDirectory() ? py::bool_(true) : py::bool_(false);
    }

    py::object is_file() {
        qDebug() << "Checking if file: " << path.c_str();
        assert(archive.isOpen());
        assert(!archive.getEntry(path).isNull());
        return archive.getEntry(path).isFile() ? py::bool_(true) : py::bool_(false);
    }

    TraversalNode joinpath(const py::args& args) {
        std::string result = path;
        for (const auto& arg : args) {
            std::string argStr = py::cast<std::string>(arg);
            if (!result.empty() && result.back() != '/') {
                result += '/';
            }
            result += argStr;
        }
        return TraversalNode(archive, result);
    }

    TraversalNode __truediv__(const py::args& args) {
        return joinpath(args);
    }

    py::object open(const std::string& mode = "r", const py::args& args = py::args(), const py::kwargs& kwargs = py::kwargs()) {
        qDebug() << "Opening file: " << path.c_str() << " with mode: " << mode.c_str();
        assert(archive.isOpen());
        auto entry = archive.getEntry(path);

        assert(!entry.isNull());
        auto size = entry.getSize();
        char* buffer = static_cast<char*>(std::malloc(size + 1));
        libzippp_uint8 *data = archive.getEntry(path).readAsBinary();
        //copy into buffer and zero terminate it
        std::memcpy(buffer, data, size);
        buffer[size] = '\0';
        auto stream = py::module_::import("io").attr("BytesIO")(py::bytes((const char*)buffer));
        if (mode == "rb") {
            return stream;
        } else if (mode == "r") {
            return py::module_::import("io").attr("TextIOWrapper")(stream, *args, **kwargs);
        } else {
            throw std::runtime_error("Unsupported mode: " + mode);
        }
    }

    std::string _path() const {
        return this->path;
    }

private:
    const std::string path;
    const ZipArchive& archive;
};

class FakeResourceReader {
   public:
    explicit FakeResourceReader(const std::string& path, const ZipArchive& archive) : path(path), archive(archive) {
    }

    TraversalNode files() {
        qDebug() << "Getting files for path: " << path.c_str();
        return TraversalNode(archive, path);
    }

    py::object open_resource(const std::string& resource) {
        qDebug() << "Opening resource: " << resource.c_str() << " for path: " << path.c_str();
        return this->files().joinpath(py::reinterpret_borrow<py::args>(py::make_tuple(resource))).open("rb", py::args(), py::kwargs());
    }

    py::object is_resource(const std::string& resource) {
        qDebug() << "Checking if resource: " << resource.c_str() << " exists for path: "
                 << path.c_str();
        return this->files().joinpath(py::reinterpret_borrow<py::args>(py::make_tuple(resource))).is_file();
    }

    std::vector<TraversalNode> contents() {
        qDebug() << "Getting contents for path: " << path.c_str();
        std::vector<TraversalNode> result;
        for (const auto& entry : this->files().iterdir()) {
            result.push_back(entry);
        }
        return result;
    }



   private:
    const std::string path;
    const ZipArchive& archive;
};

class FakePathLoader {
   public:
    explicit FakePathLoader(const QString path, const QString source, const ZipArchive& archive) : path(path), source(source), archive(archive) {
    }

    py::object create_module(const py::object& spec) {
        return py::none();
    }

    py::object exec_module(const py::object& module) {
        qDebug() << "Executing module: " << path;

        py::module_ builtins = py::module_::import("builtins");
        py::object  compile = builtins.attr("compile");
        py::object  exec = builtins.attr("exec");

        // qDebug() << "Compiling source: " << source;
        // find a null byte in the source and print the index of it
        int nullIndex = source.toStdString().find('\0');
        if (nullIndex != std::string::npos) {
            qDebug() << "Found null byte in source at index: " << nullIndex;
        }

        py::object code = compile(source.toStdString(), path.toStdString(), "exec");
        exec(code, module.attr("__dict__"));
        return py::none();
    }

    py::str get_source(const std::string& fullname) {
        return source.toStdString();
    }

    py::object get_data(const std::string& path) {
        qDebug() << "Getting data for path: " << path.c_str() << " expected: " << this->path;
        if (path == this->path.toStdString()) {
            return py::bytes(source.toStdString());
        }
        py::object OSError = py::module_::import("builtins").attr("OSError");
        throw OSError("File not found: " + path);
    }

    py::str get_filename(const std::string& fullname) {
        return path.toStdString();
    }

    FakeResourceReader get_resource_reader(const std::string& fullname) {
        qDebug() << "Getting resource reader for: " << fullname.c_str()
                 << " expected: " << this->path;
        //strip the worlds. prefix from fullname
        std::string prefix = "worlds.";
        std::string resource_path = fullname;
        if (fullname.starts_with(prefix)) {
            resource_path = fullname.substr(prefix.length());
        }
        //replace all . with / in resource_path
        std::replace(resource_path.begin(), resource_path.end(), '.', '/');
        //make sure it ends with a /
        if (!resource_path.ends_with("/")) {
            resource_path += "/";
        }
        return FakeResourceReader(resource_path, archive);
    }

   private:
    const QString path;
    const QString source;
    const ZipArchive& archive;
};

// this class must use std::string for strings because pybind11 can't reason about QStrings
class ZipModuleImporter {
   public:
    explicit ZipModuleImporter(const ZipArchive& archive, const std::string& name)
        : archive(archive), name(name) {
        assert(archive.isOpen());
        ModuleSpec = py::module_::import("importlib.machinery").attr("ModuleSpec");
    }

    py::object find_spec(const std::string& fullname, const std::optional<py::sequence>& path,
                         const std::optional<py::object>& target) {
        if (!fullname.starts_with("worlds")) {
            qDebug() << "ignoring module: " << fullname.c_str();
            return py::none();
        }

        qDebug() << "-------------------";
        qDebug() << "fullname: " << fullname;
        qDebug() << "path: "
                 << (path.has_value() ? (std::string)py::str(path.value()) : std::string("None"));

        if (fullname == "worlds") {
            py::object spec = ModuleSpec("worlds", py::none());
            spec.attr("submodule_search_locations") = py::list();
            spec.attr("submodule_search_locations").attr("append")(archive.getPath());
            spec.attr("submodule_search_locations")
                .attr("append")(
                    (QCoreApplication::applicationDirPath() + "/Archipelago/worlds").toStdString());
            qDebug() << "Returning spec for worlds: " << (std::string)py::str(spec);
            return spec;
        }

        if (!fullname.starts_with("worlds." + name)) {
            qDebug() << "ignoring module: " << fullname.c_str();
            return py::none();
        }

        // make sure that zip_worlds is in the path, if not exit early
        bool foundZipWorlds = false;
        if (path.has_value() && py::len(path.value()) > 0) {
            for (auto item : path.value()) {
                if (py::str(item).cast<std::string>().starts_with(archive.getPath())) {
                    foundZipWorlds = true;
                    break;
                }
            }
        } else {
            qWarning() << "No path provided for module: " << fullname.c_str();
            return py::none();
        }

        if (!foundZipWorlds) {
            qWarning() << ".apworld file not found in path for module: " << fullname.c_str();
            return py::none();
        }
        std::string basePath = fullname.substr(std::string("worlds.").length());
        // replace . with / in basePath
        std::replace(basePath.begin(), basePath.end(), '.', '/');

        // get the entry for the passed in path and fullname
        std::string fileEntryName = basePath + ".py";
        std::string dirEntryName = basePath + "/";
        auto        fileEntry = archive.getEntry(fileEntryName);
        if (!fileEntry.isNull() && fileEntry.isFile()) {
            qDebug() << "Found file entry for module: " << fullname.c_str()
                     << " at: " << fileEntryName.c_str();

            QString fakePath =
                QString::fromStdString(archive.getPath() + "/" + fileEntryName.c_str());
            FakePathLoader loader(
                QString::fromStdString(archive.getPath() + "/" + fileEntryName.c_str()),
                QString::fromStdString(fileEntry.readAsText()),
                archive);
            py::object spec = ModuleSpec(fullname, loader, "origin"_a = fakePath.toStdString(),
                                         "is_package"_a = false);
            spec.attr("has_location") = true;
            return spec;

        } else {
            qDebug() << "No file entry found for module: " << fullname.c_str()
                     << " at: " << fileEntryName.c_str();
        }

        auto dirEntry = archive.getEntry(dirEntryName);
        // extract and check the base directory from basePath. It should be the charaters after
        // worlds. and before the next . or end of string
        std::string baseDir = basePath.substr(0, basePath.find('/'));
        if ((!dirEntry.isNull() && dirEntry.isDirectory()) ||
            dirEntryName ==
                baseDir + "/") {  // also check for the case where the directory is the base path
            qDebug() << "Found directory entry for module: " << fullname.c_str()
                     << " at: " << dirEntryName.c_str();
            // this is a directory
            // look for if a __init__.py file exists in the directory
            std::string initFileEntryName = dirEntryName + "__init__.py";
            auto        initFileEntry = archive.getEntry(initFileEntryName);
            if (!initFileEntry.isNull() && initFileEntry.isFile()) {
                qDebug() << "Found __init__.py file for module: " << fullname.c_str()
                         << " at: " << initFileEntryName.c_str();
                // this is a package, return a spec for the package
                QString fakePath =
                    QString::fromStdString(archive.getPath() + "/" + initFileEntryName.c_str());

                FakePathLoader loader(
                    QString::fromStdString(archive.getPath() + "/" + initFileEntryName.c_str()),
                    QString::fromStdString(initFileEntry.readAsText()),
                    archive);

                py::object spec = ModuleSpec(fullname, loader, "origin"_a = fakePath.toStdString(),
                                             "is_package"_a = true);
                spec.attr("has_location") = true;
                spec.attr("submodule_search_locations") = py::list();

                spec.attr("submodule_search_locations")
                    .attr("append")(archive.getPath() + "/" + dirEntryName.c_str());

                return spec;
            } else {
                qDebug() << "No __init__.py file found for module: " << fullname.c_str()
                         << " at: " << initFileEntryName.c_str();
                // this is a namespace package, return a spec for the namespace package
                QString fakePath =
                    QString::fromStdString(archive.getPath() + "/" + dirEntryName.c_str());
                py::object spec =
                    ModuleSpec(fullname, py::none(), "origin"_a = fakePath.toStdString(),
                               "is_package"_a = true);
                spec.attr("has_location") = false;
                spec.attr("submodule_search_locations") = py::list();
                spec.attr("submodule_search_locations")
                    .attr("append")(archive.getPath() + "/" + dirEntryName.c_str());
                return spec;
            }
        } else {
            qDebug() << "No directory entry found for module: " << fullname.c_str()
                     << " at: " << dirEntryName.c_str();
            //             py::exec(R"(
            // print(repr(__name__), repr(__package__))
            // )");
        }

        // ModuleSpec.attr("origin") = archive.getPath();

        return py::none();
    }

   private:
    const ZipArchive& archive;
    const std::string name;
    py::object        ModuleSpec;
};

PYBIND11_EMBEDDED_MODULE(zipmod, m, py::mod_gil_not_used()) {
    py::class_<ZipModuleImporter>(m, "ZipModuleImporter")
        .def("find_spec", &ZipModuleImporter::find_spec);
    py::class_<FakePathLoader>(m, "FakePathLoader")
        .def("create_module", &FakePathLoader::create_module)
        .def("exec_module", &FakePathLoader::exec_module)
        .def("get_source", &FakePathLoader::get_source)
        .def("get_data", &FakePathLoader::get_data)
        .def("get_filename", &FakePathLoader::get_filename)
        .def("get_resource_reader", &FakePathLoader::get_resource_reader);

    py::class_<FakeResourceReader>(m, "FakeResourceReader")
        .def("files", &FakeResourceReader::files)
        .def("open_resource", &FakeResourceReader::open_resource)
        .def("is_resource", &FakeResourceReader::is_resource)
        .def("contents", &FakeResourceReader::contents);

    py::class_<TraversalNode>(m, "TraversalNode")
        .def("iterdir", &TraversalNode::iterdir)
        .def("is_dir", &TraversalNode::is_dir)
        .def("is_file", &TraversalNode::is_file)
        .def("joinpath", &TraversalNode::joinpath)
        .def("__truediv__", &TraversalNode::__truediv__)
        .def("open", &TraversalNode::open, py::arg("mode") = "r");
}

void getGameFromPython(const ZipArchive* archive, const QString& name) {
    PyConfig config;
    PyConfig_InitIsolatedConfig(&config);
    py::scoped_interpreter guard{&config};
    py::module_::import("zipmod");

    // get the filename minus the extension and use that as the module name from the raw path
    std::string fullPath = name.toStdString();
    std::string fileName = fullPath.substr(fullPath.find_last_of("/\\") + 1);
    std::string moduleName = fileName.substr(0, fileName.find_last_of('.'));

    ZipModuleImporter importer(*archive, moduleName);
    py::module_::import("sys").attr("meta_path").attr("insert")(0, importer);

    py::module_::import("sys").attr("path").attr("insert")(
        0, (QCoreApplication::applicationDirPath() + "/Archipelago").toStdString());
    qDebug() << "Added Archipelago to sys.path: "
                << (QCoreApplication::applicationDirPath() + "/Archipelago").toStdString().c_str();
    py::module_::import("sys").attr("path").attr("insert")(
        0,
        (QCoreApplication::applicationDirPath() + "/../3rd_party/Archipelago-deps/").toStdString());

    // need to import the Auto

    py::module_::import(std::format("worlds.{}", moduleName).c_str());

    qDebug() << "Imported module: " << moduleName.c_str();

    qDebug() << (std::string)py::str(py::module_::import("worlds").attr("AutoWorldRegister"));

    qDebug() << "Finished importing module: " << moduleName.c_str();
    // qDebug() << "game: " << py::module_::import("worlds").attr(moduleName.c_str()).
}

void APWorldWidget::installApWorld(const ZipArchive* archive, const QString& name) {
    // get the first base folder in the zip and print it out along with the name
    std::string firstEntryPath = archive->getEntries().begin()->getName();
    size_t      seporatorLocation = firstEntryPath.find_first_of('/');
    std::string firstBaseFolder = firstEntryPath.substr(0, seporatorLocation);
    // qDebug() << "First base folder in .apworld: " << firstBaseFolder << " name: " <<
    // name.toStdString().c_str();

    // look for the archipelago.json file
    ZipEntry manifest = archive->getEntry(firstBaseFolder + "/archipelago.json");
    if (manifest.isNull() || true) {
        qWarning() << "No archipelago.json file found in .apworld: " << name.toStdString().c_str();
        getGameFromPython(archive, name);
        return;
    }
    std::string manifestContents = manifest.readAsText();

    QJsonParseError parseError;
    QJsonDocument   doc = QJsonDocument::fromJson(manifestContents.data(), &parseError);
    auto            game = doc.object().constFind("game").value().toString();
    qDebug() << game;
}
