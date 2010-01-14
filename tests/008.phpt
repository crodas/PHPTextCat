--TEST--
Saving knowledge (with TextCat), and testing if it exists on the fs
--FILE--
<?php 
error_reporting(E_ALL);
@mkdir('./test/');
$tc = new TextCat();
$tc->setDirectory('./test/');
$tc->addSample("Buenos dias, esto es un ejemplo de texto");
$result = $tc->save("spanish");
$tc->addSample("This is a pretty cool sample");
$result = $tc->save("english");
unset($tc);
$tc = new TextCat();
$tc->setDirectory('./test/');
var_dump(array_search('spanish', $tc->getKnowledges()) !== false);
@unlink('./test/spanish.tc');
@unlink('./test/english.tc');
@rmdir('./test/');
?>
--EXPECT--
bool(true)
