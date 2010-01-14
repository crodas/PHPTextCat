<?php
if (!extension_loaded("textcat")) {
    die("there is not textcat\n");
}
$path = dirname(__FILE__);
$tc = new TextCat;
$tc->setDirectory($path.'/learn/');

$texts = array(
    "Hola Adrian! graciass! mucho éxito con tu compu! investigá mucho, escribí mucho y por supuesto divertite..",
    "Nueva función: responde a este mensaje para comentar este estado.",
    "For counts of detected errors, rerun with:",
);
foreach($texts as $text) {
    var_dump(array($text, $tc->getCategory($text)));
}

?>
