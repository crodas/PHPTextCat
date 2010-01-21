--TEST--
Testing Constructor/Destructors
--FILE--
<?php 
error_reporting(E_ALL);
$tc = new TextCat();
$tc->setDirectory('/tmp');
$tc->addSample("Buenos dias, esto es un ejemplo de texto");
$tc->save("spanish");
unset($tc);


for ($i=0; $i < 10000;  $i++) {
    $tc = new TextCat();
    $tc->setDirectory('/tmp');
    if ($tc->getCategory('Esto es un lindo texto') != 'spanish') {
        break;
    }
}
var_dump($i);
?>
--EXPECT--
int(10000)
