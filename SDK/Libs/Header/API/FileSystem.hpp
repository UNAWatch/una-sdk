
#pragma once

//TODO: make real wrapper

#include "Interfaces/IFileSystem.hpp"

namespace sdk::api {
    using FileSystem = Interface::IFileSystem;
    using FsObject = Interface::IFsObject;
    using File = Interface::IDirectory;
    using Directory = Interface::IDirectory;
}
