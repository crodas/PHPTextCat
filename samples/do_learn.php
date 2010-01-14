<?php
if (!extension_loaded("textcat")) {
    die("there is not textcat\n");
}
$path = dirname(__FILE__);
$tc = new TextCat;
$tc->setDirectory($path.'/learn/');

foreach (scandir($path.'/texts/') as $lang) {
    if ($lang == "." || $lang == "..") {
        continue;
    }
    $content = file_get_contents("{$path}/texts/{$lang}");
    var_dump($lang, strlen($content));
    $tc->addSample($content);
    $tc->save($lang);
}

?>
