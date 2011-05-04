#! /usr/bin/ruby

# A simple script that generates the file sets described in the SPECweb2005
# document (http://www.spec.org/web2005/docs/SupportDesign.html) on the
# current directory.
#

require "fileutils"

# Creates a file given the name and size in bytes. This overwrites the file if
# it already exists.
#
def create_file(filename, size)
  File.open(filename, "w") do |f|
    1.upto(size) do
      f.putc(rand(256))
    end
  end
end

if __FILE__ == $0 then
  FileUtils.mkdir %w(class0 class1 class2 class3 class4 class5)

  (0..4).each do |x|
    create_file("class0/file#{x}.html", (x + 1)*104858)
  end

  (0..2).each do |x|
    create_file("class1/file#{x}.html", 629146 + 125829*x)
  end

  (0..3).each do |x|
    create_file("class2/file#{x}.html", 1048576 + 492831*x)
  end

  (0..1).each do |x|
    create_file("class3/file#{x}.html", 4194304 + 1352663*x)
  end

  create_file("class4/file0.html", 9992929)
  create_file("class5/file0.html", 37748736)
end

