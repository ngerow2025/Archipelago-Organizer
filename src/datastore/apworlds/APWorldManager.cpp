#include "APWorldManager.h"

#include <sys/stat.h>

#include <QDebug>
#include <filesystem>
#include <fstream>
#include <vector>

#include "APWorldEntries_generated.h"
#include "flatbuffers/flatbuffers.h"

APWorldLocalManager::APWorldLocalManager(std::string basePath) : basePath(std::move(basePath)) {
}

APWorldLocalManager::~APWorldLocalManager() = default;

void APWorldLocalManager::loadAPWorldEntries() {
    // Load {basePath}/{APWORLD_DIRECTORY}/apworld_entries.bin
    std::string filePath = basePath + "/" + APWORLD_DIRECTORY + "/apworld_entries.bin";

    // stat the file to get length and if it exists
    struct stat st;
    if (stat(filePath.c_str(), &st) != 0) {
        // file does not exist, return
        this->apWorldEntries.clear();
        qDebug() << "APWorldLocalManager::loadAPWorldEntries: file does not exist: "
                 << QString::fromStdString(filePath);
        return;
    }

    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        this->apWorldEntries.clear();
        qWarning() << "APWorldLocalManager::loadAPWorldEntries: could not open file: "
                   << QString::fromStdString(filePath);
        return;
    }

    std::vector<char> buffer(static_cast<size_t>(st.st_size));
    if (!file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()))) {
        this->apWorldEntries.clear();
        qWarning() << "APWorldLocalManager::loadAPWorldEntries: could not read file: "
                   << QString::fromStdString(filePath);
        return;
    }

    file.close();

    // Always verify a buffer read from disk before accessing it.
    flatbuffers::Verifier verifier(reinterpret_cast<const uint8_t*>(buffer.data()), buffer.size());
    if (!serialization::VerifyAPWorldEntriesBuffer(verifier)) {
        this->apWorldEntries.clear();
        qWarning() << "APWorldLocalManager::loadAPWorldEntries: invalid flatbuffer: "
                   << QString::fromStdString(filePath);
        return;
    }

    const auto* root = serialization::GetAPWorldEntries(buffer.data());

    this->apWorldEntries.clear();
    if (root->entries() == nullptr) {
        return;
    }

    this->apWorldEntries.reserve(root->entries()->size());
    for (const auto* entry : *root->entries()) {
        this->apWorldEntries.emplace_back(
            entry->major_version(), entry->minor_version(), entry->patch_version(),
            entry->game() ? entry->game()->str() : std::string(),
            entry->remote_url() ? entry->remote_url()->str() : std::string(),
            entry->local_path() ? entry->local_path()->str() : std::string(),
            static_cast<LocalSourceType>(entry->local_source_type()),
            static_cast<RemoteSourceType>(entry->remote_source_type()));
    }
}

void APWorldLocalManager::saveAPWorldEntries() {
    std::filesystem::path directory = std::filesystem::path(basePath) / APWORLD_DIRECTORY;
    std::error_code       ec;
    std::filesystem::create_directories(directory, ec);
    if (ec) {
        qWarning() << "APWorldLocalManager::saveAPWorldEntries: could not create directory: "
                   << QString::fromStdString(directory.string()) << ": "
                   << QString::fromStdString(ec.message());
        return;
    }

    flatbuffers::FlatBufferBuilder                                builder;
    std::vector<flatbuffers::Offset<serialization::APWorldEntry>> entryOffsets;
    entryOffsets.reserve(apWorldEntries.size());

    for (const auto& entry : apWorldEntries) {
        entryOffsets.push_back(serialization::CreateAPWorldEntryDirect(
            builder, entry.majorVersion, entry.minorVersion, entry.patchVersion, entry.game.c_str(),
            entry.remoteUrl.c_str(), entry.localPath.c_str(),
            static_cast<serialization::LocalSourceType>(entry.localSourceType),
            static_cast<serialization::RemoteSourceType>(entry.remoteSourceType)));
    }

    auto entries = builder.CreateVector(entryOffsets);
    auto root = serialization::CreateAPWorldEntries(builder, entries);
    builder.Finish(root);

    std::ofstream file(directory / "apworld_entries.bin", std::ios::binary | std::ios::trunc);
    if (!file) {
        qWarning() << "APWorldLocalManager::saveAPWorldEntries: could not open file for writing";
        return;
    }

    file.write(reinterpret_cast<const char*>(builder.GetBufferPointer()),
               static_cast<std::streamsize>(builder.GetSize()));

    file.close();
    
    if (file.fail()) {
        qWarning()
            << "APWorldLocalManager::saveAPWorldEntries: error occurred while writing to file";
    }
};
