#! /usr/bin/ruby

# A simple script for generating the replay log for httperf. The replay log
# follows the file distribution described in the SPECweb2005 document
# (http://www.spec.org/web2005/docs/SupportDesign.html)
#

def file_list
  list = []

  list.concat(Array.new(4, "class0/file0.html"))
  list << "class0/file1.html"
  list.concat(Array.new(2, "class0/file2.html"))
  list.concat(Array.new(3, "class0/file3.html"))
  list.concat(Array.new(4, "class0/file4.html"))

  list.concat(Array.new(7, "class1/file0.html"))
  list.concat(Array.new(2, "class1/file1.html"))
  list.concat(Array.new(3, "class1/file2.html"))

  list.concat(Array.new(8, "class2/file0.html"))
  list.concat(Array.new(5, "class2/file1.html"))
  list.concat(Array.new(5, "class2/file2.html"))
  list.concat(Array.new(10, "class2/file3.html"))

  list.concat(Array.new(15, "class3/file0.html"))
  list.concat(Array.new(7, "class3/file1.html"))

  list.concat(Array.new(12, "class4/file0.html"))

  list.concat(Array.new(11, "class5/file0.html"))

  return list.shuffle
end

if __FILE__ == $0 then
  if ARGV.size != 1 then
    puts "usage: ./#{$0} <output filename>"
  else
    filename = ARGV[0]

    File.open(filename, "w") do |f|
      file_list.each do |filename|
        f.write filename
        f.putc 0 # ASCII NULL character
      end
    end
  end
end

