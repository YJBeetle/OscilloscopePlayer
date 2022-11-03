cmake_minimum_required(VERSION 3.5)

project(OscilloscopePlayer VERSION 1.0 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Multimedia)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Multimedia)
find_path(AVFORMAT_INCLUDE_DIR libavformat/avformat.h)
find_library(AVFORMAT_LIBRARY avformat)
find_path(AVCODEC_INCLUDE_DIR libavcodec/avcodec.h)
find_library(AVCODEC_LIBRARY avcodec)
find_path(AVUTIL_INCLUDE_DIR libavutil/avutil.h)
find_library(AVUTIL_LIBRARY avutil)
find_path(SWSCALE_INCLUDE_DIR libswscale/swscale.h)
find_library(SWSCALE_LIBRARY swscale)
find_path(SWRESAMPLE_INCLUDE_DIR libswresample/swresample.h)
find_library(SWRESAMPLE_LIBRARY swresample)

set(PROJECT_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    videoviewer.cpp
    videoviewer.h
    decode.cpp
    decode.h
    oscilloscope.cpp
    oscilloscope.h
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(${PROJECT_NAME}
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET ${PROJECT_NAME} APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(${PROJECT_NAME} SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(${PROJECT_NAME}
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Multimedia)
target_include_directories(${PROJECT_NAME} PRIVATE ${AVFORMAT_INCLUDE_DIR} ${AVCODEC_INCLUDE_DIR} ${AVUTIL_INCLUDE_DIR} ${SWSCALE_INCLUDE_DIR} ${SWRESAMPLE_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE ${AVFORMAT_LIBRARY} ${AVCODEC_LIBRARY} ${AVUTIL_LIBRARY} ${SWSCALE_LIBRARY} ${SWRESAMPLE_LIBRARY})

set_target_properties(${PROJECT_NAME} PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER ${PROJECT_NAME}.yjbeetle.art
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

install(TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(${PROJECT_NAME})
endif()

# Deploy Qt
if(NOT CMAKE_BUILD_TYPE MATCHES Debug)
    get_target_property(_qmake_executable Qt${QT_VERSION_MAJOR}::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
    if(ANDROID)
        # https://doc.qt.io/qt-6/deployment-android.html
        # https://doc.qt.io/qt-6/android-deploy-qt-tool.html
        # https://doc.qt.io/qt-6/qt-android-add-apk-target.html
        # https://doc.qt.io/qt-6/qt-android-generate-deployment-settings.html
        # https://doc.qt.io/qtcreator/creator-deploying-android.html
        set(QT_NO_GLOBAL_APK_TARGET_PART_OF_ALL, ON)
        find_program(DEPLOYQT_EXECUTABLE NAMES androiddeployqt HINTS "${_qt_bin_dir}")
        add_custom_command(TARGET ${PROJECT_NAME}
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/android-build/libs/${CMAKE_ANDROID_ARCH_ABI}/"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:${PROJECT_NAME}>" "${CMAKE_BINARY_DIR}/android-build/libs/${CMAKE_ANDROID_ARCH_ABI}/"
            COMMAND ${DEPLOYQT_EXECUTABLE}
                --input "${CMAKE_BINARY_DIR}/android-${PROJECT_NAME}-deployment-settings.json"
                --output "${CMAKE_BINARY_DIR}/android-build"
                --android-platform android-31
                --gradle
                --sign ${CMAKE_SOURCE_DIR}/AndroidKeystore/release.keystore release --storepass 000000
        )
    elseif(APPLE)
        find_program(DEPLOYQT_EXECUTABLE NAMES macdeployqt HINTS "${_qt_bin_dir}")
        add_custom_command(TARGET ${PROJECT_NAME}
            POST_BUILD
            COMMAND ${DEPLOYQT_EXECUTABLE}
                "$<TARGET_BUNDLE_DIR:${PROJECT_NAME}>"
                -qmldir=${CMAKE_SOURCE_DIR}
        )
    elseif(WIN32)
        find_program(DEPLOYQT_EXECUTABLE NAMES windeployqt HINTS "${_qt_bin_dir}")
        add_custom_command(TARGET ${PROJECT_NAME}
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:${PROJECT_NAME}>" "${CMAKE_BINARY_DIR}/${PROJECT_NAME}_output/"
            COMMAND ${DEPLOYQT_EXECUTABLE}
                --dir "${CMAKE_BINARY_DIR}/${PROJECT_NAME}_output/"
                "$<TARGET_FILE:${PROJECT_NAME}>"
                --$<LOWER_CASE:$<CONFIG>>
                --qmldir ${CMAKE_SOURCE_DIR}
        )
    endif()
endif()
