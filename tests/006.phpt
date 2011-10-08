--TEST--
Extending and Implementing our own _save
--FILE--
<?php 
error_reporting(E_ALL);

final class PHP_TextCat extends BaseTextCat
{
    function _save($name, Array $arr)
    {
        var_dump($name, is_array($arr)&&count($arr) > 0);
    }

    function _load()
    {
    }

    function _list()
    {
    }
}
$tc = new PHP_TextCat();
$tc->addSample("Buenos dias, esto es un ejemplo de texto");
$tc->save("spanish");
?>
--EXPECT--
string(7) "spanish"
bool(true)

