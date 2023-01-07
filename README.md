# tcp_bank_client_server

TCP Server ve Client Visual Studio 2022'de Windows üzerinde çalışması için yazıldı.
g++ on command line üzerinden derlenemedi. (mutex and thread header dosyaları default kütüphanede olmadığı için bulamıyor)
Visual Studio 2022 ile derleme ve çalıştırmada bir problem ile karşılaşılmadı.

--SERVER--

Server önce .txt dosyasından kullanıcı bilgilerini bir unordered_map yapısına kaydediyor.
Kullanıcılar ID password name surname bank balance yapısından oluşuyor.
ID aynı zamanda map için key olarak kullanılıyor. 

Listening socket açıp 20000 numaralı portu dinliyor.
Gelen her client bağlantı isteği için bir thread oluşturuyor.
Bu sayede aynı anda birden fazla client bağlanıp işlem yapabiliyor. (Şuan max 10 client için thread oluşturuyor, fazlası aktif thread sonlanmasını bekliyor.)

Oluşan her thread client üzerinden gelen istekleri alıp işleyip cevap gönderiyor.
unordered_map üzerinde birden fazla thread aynı anda değişiklik yapabilsin diye mutex kullanılıyor.

login olanların listesini tutmak için bir set kullanılıyor.
Örnek olarak login ve logout için islogged kullanılıyor.
login için islogged true gönderse zaten giriş yapmış deniyor. (Farklı client programları aynı kişi için giriş yapamasın diye konuldu.)
logout için islogged false ise giriş yapılmadığı için çıkış yapamadı deniyor.

Server kapanması için bir thread sürekli okuma yapıyor.
command line üzerinden EXIT yazılırsa listening socket'i kapatıp threadlerin sonlanmasını bekleyip, güncel verileri dosyaya yazdırıp programı sonlandırıyor.
Bu şekilde sonlandırma, accept fonksiyonunun blocking olmasından dolayı yapıldı.
Listening socket kapatıldığında accept fonksiyonu da hata döndürüp while döngüsünden çıkılarak programın sonlanabilmesini sağlıyor.

--CLIENT--

Client çalıştırıldığında server'a bağlanıyor ve kullanıcı adı - şifre istiyor.
(login olmak için user.txt'den kullanıcı adları ve şifrelere bakılabilir)
Login başarılı olursa bir kullanım talimatı yazdırılıyor ve operation girilmesi bekleniyor.
Yanlış komut girilirse program sonlandırılıyor (şimdilik bu şekilde bıraktım.)
Doğru komut girilirse server'a ilgili istek gönderiliyor ve cevap bekleniyor.
Gelen cevap SUCCESS ise işlem başarılı diyerek güncel bakiye görüntüleniyor.
Gelen cevap FAIL ise işlem başarısız deniyor ve yeni işlem bekleniyor.

LOGIN işlemi sadece girişte girilen kullanıcı adı ve şifre için gönderiliyor.
Başarılı login işlemi sonrasında kullanıcı
DEPOSIT - WITHDRAW - TRANSFER - LOGOUT komutlarını kullanabiliyor. Komutlar için gerekli parametreler de ekrana yazdırılıyor.

LOGOUT komutuna SUCCESS dönerse program (client) sonlandırılıyor.
