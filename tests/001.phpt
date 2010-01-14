--TEST--
Check for textcat presence
--SKIPIF--
<?php if (!extension_loaded("textcat")) print "skip"; ?>
--FILE--
<?php 
echo "textcat extension is available";
?>
--EXPECT--
textcat extension is available
