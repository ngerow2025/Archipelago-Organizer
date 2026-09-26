#pragma once

#undef slots
#include <pybind11/embed.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <QString>
#include <libzippp.h>

using namespace libzippp;


void getGameFromPython(const ZipArchive* archive, const QString& name);