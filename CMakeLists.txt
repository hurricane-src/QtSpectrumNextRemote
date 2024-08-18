cmake_minimum_required(VERSION 3.30)
project(QtSpectrumNextRemote)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)


find_package(Qt6 COMPONENTS
        Core
        Gui
        Widgets
        Network
        REQUIRED)

add_executable(QtSpectrumNextRemote
        main.cpp
        NEX.cpp NEX.h
        #SpectrumNextRemote.cpp SpectrumNextRemote.h
        SpectrumNextRemoteForm.cpp SpectrumNextRemoteForm.h SpectrumNextRemoteForm.ui
        RemoteCommand.h)

target_link_libraries(QtSpectrumNextRemote
        Qt::Core
        Qt::Gui
        Qt::Widgets
        Qt::Network)

add_executable(QtSpectrumNextRemoteCLI
        main-cli.cpp
        NEX.cpp NEX.h
        QtSpectrumNextRemoteTask.cpp
        QtSpectrumNextRemoteTask.h)

target_link_libraries(QtSpectrumNextRemoteCLI
        Qt::Core
        Qt::Network)
