# Studi Kasus: Relevansi Insiden Perlintasan Mengkreng terhadap Project Palang Pintu Kereta Otomatis Berbasis IoT

## 1. Ringkasan Kejadian

Pada Rabu, 22 Juli 2026 pukul 01.48 WIB, palang pintu perlintasan sebidang di JPL 303A KM 213+705, petak jalan Kertosono–Papar (Perlintasan Mengkreng, Kecamatan Purwoasri, Kabupaten Kediri, Jawa Timur) tidak menutup saat KA 152 Brantas relasi Pasar Senen–Blitar melintas. PT KAI Daop 7 Madiun membenarkan bahwa penyebabnya adalah petugas jaga perlintasan yang diduga tertidur dan tidak menjalankan prosedur operasional standar. Kejadian sempat menimbulkan kepanikan warga dan pengguna jalan — beberapa warga bahkan turun ke jalan untuk menghentikan kendaraan secara manual — namun beruntung tidak ada korban jiwa maupun kecelakaan. Petugas yang bersangkutan dijatuhi Surat Peringatan (SP), dan KAI menyampaikan permintaan maaf resmi kepada masyarakat sambil berjanji memperkuat disiplin dan pengawasan petugas perlintasan.

**Sumber:** Kompas.com, "KAI Akui Petugas Tertidur saat KA Brantas Melintas: Kami Minta Maaf", 22 Juli 2026.

## 2. Analisis Akar Masalah (Root Cause)

| Aspek | Temuan pada Kasus Nyata |
|---|---|
| Jenis perlintasan | Sebidang, dijaga manual oleh petugas |
| Mekanisme penutupan palang | Manual, sepenuhnya bergantung pada kesigapan dan kesadaran petugas |
| Titik gagal (failure point) | Human error — petugas tertidur saat bertugas |
| Sistem cadangan otomatis | Tidak ada |
| Dampak | Nyaris kecelakaan, kepanikan warga, kerugian reputasi bagi operator |

Akar masalahnya bukan pada infrastruktur rel atau sinyal kereta, melainkan pada **satu titik kegagalan tunggal (single point of failure)**: keandalan sistem sepenuhnya bertumpu pada kondisi fisik dan kewaspadaan satu orang petugas, tanpa mekanisme otomatis yang mendeteksi kedatangan kereta secara independen dan menutup palang tanpa campur tangan manusia.

## 3. Relevansi terhadap Project

Project palang pintu kereta otomatis 2 bidang berbasis IoT yang sedang dikembangkan justru dirancang untuk menutup celah kegagalan yang sama persis dengan yang terjadi di Perlintasan Mengkreng:

| Masalah pada Kasus Nyata | Solusi pada Sistem yang Dirancang |
|---|---|
| Penutupan palang bergantung pada petugas manusia yang bisa lengah/tertidur | Deteksi kereta dilakukan otomatis oleh sensor HC-SR04, tidak memerlukan kehadiran/kesadaran manusia |
| Tidak ada mekanisme peringatan dini otomatis | Fase WARNING otomatis (lampu kuning berkedip + buzzer) begitu kereta terdeteksi pada jarak konseptual 50 meter |
| Waktu penutupan tidak konsisten/tidak terukur | Waktu tutup dihitung otomatis dari rumus jarak/kecepatan, bukan estimasi manual |
| Tidak ada pengawasan/monitoring dari pihak luar terhadap kondisi perlintasan | ESP8266 mengirim status real-time (IDLE/WARNING/CROSSING) ke dashboard IoT yang bisa dipantau dari jarak jauh oleh operator/pengawas |
| Insiden baru diketahui setelah viral di media sosial | Dengan logging status via IoT, kondisi perlintasan dapat dipantau dan diaudit kapan saja tanpa menunggu laporan warga |
| Tidak ada indikator visual berapa lama lagi penutupan berlangsung | TM1637 menampilkan hitung mundur waktu lintas secara jelas bagi pengguna jalan |

## 4. Argumentasi Studi Kasus untuk Laporan/Sidang

Poin ini bisa digunakan sebagai **latar belakang masalah (justifikasi)** dalam laporan/proposal:

1. **Bukti nyata urgensi otomatisasi.** Insiden Mengkreng membuktikan bahwa sistem perlintasan yang sepenuhnya bergantung pada kewaspadaan manusia memiliki risiko kegagalan nyata, bukan sekadar asumsi teoretis.
2. **Human error tidak bisa dihilangkan, tapi bisa dimitigasi dengan otomatisasi.** KAI sendiri mengakui pihaknya "tidak mentoleransi" pelanggaran prosedur, namun sanksi administratif (SP) sifatnya reaktif — terjadi setelah insiden. Sistem otomatis bersifat preventif karena tidak mengenal kondisi lelah, mengantuk, atau lalai.
3. **Konsistensi dan objektivitas.** Sensor tidak memiliki variabilitas performa seperti manusia (kondisi fisik, jam kerja, dsb.) sehingga waktu respons antar-kejadian jauh lebih konsisten.
4. **Transparansi dan akuntabilitas.** Dashboard IoT memberi jejak status yang bisa diaudit, berbeda dengan kasus nyata yang baru terungkap setelah video viral di media sosial.

## 5. Keterbatasan yang Perlu Diakui secara Jujur

Supaya studi kasus ini kredibel di hadapan penguji, penting juga mengakui batasan sistem:

- **Skala simulasi, bukan implementasi nyata.** HC-SR04 hanya mampu mendeteksi objek hingga ±4 meter, sehingga "jarak 50 meter" dalam project ini adalah parameter simulasi untuk perhitungan waktu, bukan pengukuran jarak sesungguhnya. Sistem nyata KAI menggunakan track circuit/axle counter yang dapat mendeteksi kereta dari jarak sebenarnya (ratusan meter hingga kilometer).
- **Bukan pengganti total peran manusia.** Dalam pernyataan resminya, Manager Humas KAI Daop 7 Madiun juga menegaskan bahwa palang pintu bukanlah alat pengaman utama, melainkan alat bantu — pengguna jalan tetap wajib waspada. Prinsip ini tetap berlaku meski sistem sudah otomatis: otomatisasi mengurangi risiko human error dari sisi **petugas**, tapi kewaspadaan pengguna jalan tetap menjadi lapisan keselamatan terakhir.
- **Perlu redundansi sensor pada implementasi nyata.** Sistem produksi idealnya memakai lebih dari satu sensor/metode deteksi (misalnya kombinasi sensor jarak + sensor rel) supaya jika satu sensor gagal, ada cadangan — sama seperti prinsip yang gagal diterapkan pada sisi manusia di kasus Mengkreng (tidak ada shift/pengawasan cadangan saat petugas tertidur).

## 6. Kesimpulan

Insiden Perlintasan Mengkreng adalah contoh kasus nyata dan aktual (Juli 2026) yang memperkuat urgensi project palang pintu otomatis berbasis IoT. Sistem yang dirancang — dengan deteksi otomatis via HC-SR04, perhitungan waktu tutup otomatis berbasis jarak/kecepatan, serta monitoring real-time via ESP8266 — secara langsung menjawab akar masalah pada kasus tersebut, yaitu ketergantungan penuh pada kewaspadaan manusia tanpa mekanisme otomatis sebagai lapisan keselamatan tambahan.

---
**Referensi:**
Kompas.com. (22 Juli 2026). *KAI Akui Petugas Tertidur saat KA Brantas Melintas: Kami Minta Maaf*. https://surabaya.kompas.com/read/2026/07/22/135801378/kai-akui-petugas-tertidur-saat-ka-brantas-melintas-kami-minta-maaf
