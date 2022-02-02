from conans import ConanFile, CMake, tools
import re
from os import path

class StructuredConcurrencyExampleRecipe(ConanFile):
   name = "Structured Concurrency Example"
   description = "example code for using structure concurrency with senders/receivers"
   author = "Lucian Radu Teodorescu"
   topics = ("C++", "concurrency")
   homepage = "https://github.com/lucteo/structured_concurrency_example"
   url = "https://github.com/lucteo/structured_concurrency_example"
   license = "MIT License"

   settings = "os", "compiler", "build_type", "arch"
   generators = "cmake"
   build_policy = "missing"   # Some of the dependencies don't have builds for all our targets

   options = {"shared": [True, False], "fPIC": [True, False]}
   default_options = {"shared": False, "fPIC": True}

   exports_sources = ("include/*", "CMakeLists.txt")

   def set_version(self):
      self.version = "0.1.0"

   def build_requirements(self):
      self.build_requires("asio/1.21.0")

   def config_options(self):
       if self.settings.os == "Windows":
           del self.options.fPIC

   def build(self):
      # Note: options "shared" and "fPIC" are automatically handled in CMake
      cmake = self._configure_cmake()
      cmake.build()

   def package(self):
      cmake = self._configure_cmake()
      cmake.install()

   def package_info(self):
      self.cpp_info.libs = self.collect_libs()

   def _configure_cmake(self):
      cmake = CMake(self)
      if self.settings.compiler == "Visual Studio" and self.options.shared:
         cmake.definitions["CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS"] = True
      cmake.configure(source_folder=None)
      return cmake


