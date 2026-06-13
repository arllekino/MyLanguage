cmake_minimum_required(VERSION 3.30)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
project(MyLanguage)

set(CMAKE_CXX_STANDARD 20)

include_directories(glad/include)
include(FetchContent)
FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.5.0
)
FetchContent_MakeAvailable(Catch2)

FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3.3.8
)
FetchContent_Declare(
        glad
        GIT_REPOSITORY https://github.com/Dav1dde/glad.git
        GIT_TAG v0.1.36
)

set(GLAD_INSTALL OFF CACHE BOOL "" FORCE)
set(GLAD_GENERATOR "" CACHE STRING "" FORCE)
set(GLAD_GENERATOR "c" CACHE STRING "Language generator for GLAD" FORCE)

FetchContent_MakeAvailable(glad)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(glfw)

find_package(OpenGL REQUIRED)
find_package(glm REQUIRED)
find_package(GLEW REQUIRED)

add_executable(MyLanguage
        src/main.cpp
        src/VirtualMachine/Value/Value.h
        src/VirtualMachine/OpCode.h
        src/VirtualMachine/Chunk.h
        src/VirtualMachine/VirtualMachine.h
        src/VirtualMachine/Value/ValueUtilsForLogic.h
        src/VirtualMachine/Value/ValueUtilsForPrint.h
        src/VirtualMachine/ByteCodeParser.h
        src/Lexer/Lexer.h
        src/Lexer/TokenType.h
        src/Lexer/Token.h
        src/ASTBuilder/ASTBuilder.h
        src/ASTBuilder/AST.h
        src/Compiler/Compiler.h
        src/Utils/Parameter.h
        src/Compiler/ByteCodeExporter.h
        src/Utils/TypeInfo.h
        src/Signature/Signature.h
        src/ASTBuilder/AccessLevel.h
        src/Graphics/UIRenderer.h
        src/Graphics/View/ViewNode.h
        src/Graphics/View/VBoxNode.h
        src/Graphics/View/Shapes/Rect.h
)

target_link_libraries(MyLanguage
        PRIVATE
        glfw
        OpenGL::GL
        glm::glm
        GLEW::GLEW
        glad
)

add_executable(MyLanguageTest test/test.cpp)

target_link_libraries(MyLanguageTest PRIVATE Catch2::Catch2WithMain)

include(CTest)
include(Catch)
catch_discover_tests(MyLanguageTest)