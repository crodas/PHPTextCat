--TEST--
Testing final methods
--FILE--
<?php 
class TextCat extends BaseTextCat 
{
    function getInfo() { }
}

?>
--EXPECTF--
Fatal error: Cannot override final method BaseTextCat::getInfo() in %s on line %d
