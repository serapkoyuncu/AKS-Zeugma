using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.IO.Ports;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.Windows.Forms.DataVisualization.Charting;
using System.Runtime.InteropServices;

namespace Zeugma_
{
    public partial class Form1 : Form
    {
        string[] portlar = SerialPort.GetPortNames();
        List<ProgressBar> progressBars = new List<ProgressBar>();
        List<Label> voltageLabels = new List<Label>();
        private StreamWriter writer;
        private DateTime startTime;
        private Timer dataCheckTimer;
        private DateTime lastDataReceivedTime;
        private long millisecondsSinceStart;
        private long counter = 0;
        private DateTime counterTime;
        private const int maxMissedDataThreshold = 3;
   
        public Form1()
        {
            InitializeComponent();
            InitializeProgressBarsAndLabels();
            InitializeDataCheckTimer();
            InitializeChart();
           

    }

        private void Form1_Load(object sender, EventArgs e)
        {
            foreach (string port in portlar)
            {
                comboBox1.Items.Add(port);
            }
            comboBox1.SelectedIndex = portlar.Length > 0 ? 0 : -1;

            comboBox2.Items.AddRange(new string[] { "4800", "9600" });
            comboBox2.SelectedIndex = 1;

            label2.Text = "Bağlantı Kapalı";
            
        }

        private void button1_Click(object sender, EventArgs e)
        {
          
            if (serialPort1.IsOpen)
            {
                serialPort1.Close();
                timer1.Stop();
                dataCheckTimer.Stop(); // Data check timer'ı durdur
                writer?.Close(); // Dosya yazıcıyı kapat
                label2.Text = "Bağlantı Kapalı";
               
            }
            else
            {

                try
                {
                    serialPort1.PortName = comboBox1.SelectedItem.ToString();
                    serialPort1.BaudRate = int.Parse(comboBox2.SelectedItem.ToString());
                    serialPort1.Open();
                    timer1.Start();
                    dataCheckTimer.Start(); // Data check timer'ı başlat
                    CreateNewLogFile(); // Yeni bir dosya oluştur
                    lastDataReceivedTime = DateTime.Now; // Son veri alım zamanını güncelle
                    label2.Text = "Bağlantı Açık";
                   
                    
                }
                catch (Exception ex)
                {
                    MessageBox.Show("Seri port açılamadı: " + ex.Message, "Hata", MessageBoxButtons.OK, MessageBoxIcon.Error);
                }
            }

        }
        private void button2_Click(object sender, EventArgs e)
        {

            if (serialPort1.IsOpen)
            {

                serialPort1.Close();
                timer1.Stop();

                dataCheckTimer.Stop(); // Zamanlayıcıyı durdur
                writer?.Close(); // Dosya yazıcısını kapat
                label2.Text = "Bağlantı Kapalı";
               
               
            }
            else
            {
                MessageBox.Show("Seri port zaten kapalı.", "Uyarı", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            }
        }



        private void InitializeProgressBarsAndLabels()
        {
            for (int i = 0; i < 20; i++)
            {
                ProgressBar progressBar = this.Controls.Find("verticalProgressBar" + (i + 1), true).FirstOrDefault() as ProgressBar;
                Label voltageLabel = this.Controls.Find("label" + (i + 8), true).FirstOrDefault() as Label;

                if (progressBar != null && voltageLabel != null)
                {
                    progressBars.Add(progressBar);
                    voltageLabels.Add(voltageLabel);
                }
                else
                {
                    MessageBox.Show("ProgressBar veya Label bulunamadı: " + (i + 1));
                }
            }

            // Kontrollerin doğru sayıda yüklendiğini kontrol edelim
            Debug.Assert(progressBars.Count == 20, "ProgressBars listesi 20 eleman içermiyor.");
            Debug.Assert(voltageLabels.Count == 20, "VoltageLabels listesi 20 eleman içermiyor.");
        }

        private void InitializeDataCheckTimer()
        {
            dataCheckTimer = new Timer();
            dataCheckTimer.Interval = 2000; 
            dataCheckTimer.Tick += DataCheckTimer_Tick;
        }
        private void DataCheckTimer_Tick(object sender, EventArgs e)
        {
            //if ((DateTime.Now - counterTime ).TotalSeconds > 10)
            //{
            //    writer = null;
            //}
        }
       
      

        private void InitializeChart()
        {
            ChartArea chartArea = new ChartArea("Sıcaklık Alanı");
            chart1.ChartAreas.Add(chartArea);

            Series series = new Series("Sıcaklık");
            series.ChartType = SeriesChartType.Spline; // Spline grafiği
            series.BorderWidth = 2; // Çizgi kalınlığı
            series.Color = Color.Red; // Çizgi rengi
            chart1.Series.Add(series);

            // X ekseni
            chart1.ChartAreas[0].AxisX.LabelStyle.Format = "HH:mm:ss";
            chart1.ChartAreas[0].AxisX.IntervalType = DateTimeIntervalType.Seconds;
            chart1.ChartAreas[0].AxisX.Interval = 1;
            chart1.ChartAreas[0].AxisX.ScaleView.Zoomable = true; // X ekseninde yakınlaştırma
            chart1.ChartAreas[0].AxisX.MajorGrid.Enabled = false; // X ekseni ana ızgarası devre dışı
            chart1.ChartAreas[0].AxisX.Minimum = DateTime.Now.ToOADate(); // Başlangıç zamanı

            // Y ekseni
            chart1.ChartAreas[0].AxisY.Minimum = 0; // Minimum sıcaklık değeri
            chart1.ChartAreas[0].AxisY.Maximum = 100; // Maksimum sıcaklık değeri
            chart1.ChartAreas[0].AxisY.Interval = 5; 
        }

        private void timer1_Tick(object sender, EventArgs e)
        {
            
            try
            {
                if (serialPort1.IsOpen)
                {
                    counter++;
                    
                    counterTime = DateTime .Now;

                    // Seri porttan gelen veriyi oku
                    string sonuc = serialPort1.ReadLine().Trim(); // Trim boşlukları temizler
                   
                  
                    label31.Text = $"counter: {counter}";
                    // Veriyi ',' ile ayır ve parçaları al
                    string[] veriParcalari = sonuc.Split(',');

                    // Veri uzunluğunu kontrol et
                    Debug.Assert(veriParcalari.Length == 24, "Seri port verisi beklenen uzunlukta değil: " + sonuc);

                    if (veriParcalari.Length == 24) // Hız, sıcaklık, 20 voltaj verileri ve enerji verisini içeriyorsa
                    {
                        // Hız verisini dönüştür
                        if (int.TryParse(veriParcalari[0], out int hiz))
                        {
                            // Sıcaklık verisini dönüştür
                            string sicaklikStr = veriParcalari[1].Replace('.', ','); // Nokta varsa virgül yap
                            if (float.TryParse(sicaklikStr, out float sicaklik))
                            {
                                // UI güncellemelerini ana iş parçacığına yönlendirin
                                this.Invoke((MethodInvoker)delegate
                                {
                                    // Hız verilerini güncelle
                                    label6.Text = $"Hız: {hiz}";
                                    aGauge1.Value = hiz;

                                    // Sıcaklık verilerini güncelle
                                    label7.Text = $"Sıcaklık: {sicaklik:F2} °C";
                                    UpdateGraph(sicaklik);

                                    // Voltaj verilerini güncelle
                                    for (int i = 0; i < 20; i++)
                                    {
                                        string voltajStr = veriParcalari[i + 2].Replace('.', ',');
                                        if (float.TryParse(voltajStr, out float voltaj))
                                        {
                                            UpdateVoltageBar(i, voltaj);
                                        }
                                        else
                                        {
                                            voltageLabels[i].Text = "Geçersiz voltaj verisi";
                                        }
                                    }
                                    string toplamVoltajStr = veriParcalari[22].Replace('.', ',');
                                    if (float.TryParse(toplamVoltajStr, out float toplamVoltaj))
                                    {
                                        label32.Text = $"Toplam Voltaj: {toplamVoltaj:F2} V";
                                    }
                                    else
                                    {
                                        label32.Text = "Geçersiz toplam voltaj verisi";
                                    }
                                    // Kalan enerji verisini güncelle
                                    if (int.TryParse(veriParcalari[23], out int kalanEnerji))
                                    {
                                        circularProgressBar1.Value = kalanEnerji;
                                        circularProgressBar1.Text = $"{kalanEnerji} ";
                                        label30.Text = $"{kalanEnerji} Wh";
                                    }
                                    else
                                    {
                                        label30.Text = "Geçersiz enerji verisi";
                                    }
                                    SaveDataToFile(veriParcalari, hiz, sicaklik);
                                    lastDataReceivedTime = DateTime.Now; // Son veri alım zamanını güncelle
                                    
                                });
                            }
                            else
                            {
                                label6.Text = "Geçersiz sıcaklık verisi: " + sonuc;
                            }
                        }
                        else
                        {
                            label6.Text = "Geçersiz hız verisi: " + sonuc;
                        }
                    }
                    else
                    {
                        label6.Text = "Beklenmeyen veri formatı: " + sonuc;
                    }
                }
                else
                {
                    MessageBox.Show("Seri port açık değil.", "Hata", MessageBoxButtons.OK, MessageBoxIcon.Error);
                    timer1.Stop();
                }
            }
            catch (TimeoutException)
            {
                
                
            }
            catch (InvalidOperationException ex)
            {
                // Seri port kapalıyken okuma yapılmaya çalışıldığında bu hata olabilir
                MessageBox.Show("Seri port hatası: " + ex.Message, "Hata", MessageBoxButtons.OK, MessageBoxIcon.Error);
                timer1.Stop();
            }
            catch (IOException ex)
            {
                // Seri portta bir I/O hatası meydana geldiğinde bu hata olabilir
                MessageBox.Show("Seri port I/O hatası: " + ex.Message, "Hata", MessageBoxButtons.OK, MessageBoxIcon.Error);
                timer1.Stop();
            }
            catch (Exception ex)
            {
                MessageBox.Show("Bir hata oluştu: " + ex.Message, "Hata", MessageBoxButtons.OK, MessageBoxIcon.Error);
                timer1.Stop();
            }

        }


        private void UpdateGraph(float sicaklik)
        { // Invoke kullanarak grafiği güncelleme işlemini ana iş parçacığına yönlendirin
            this.Invoke((MethodInvoker)delegate
            {
                // Grafik serisini al
                Series series = chart1.Series["Sıcaklık"];

                // Zaman damgasını al
                DateTime zaman = DateTime.Now;

                // Yeni veriyi ekleyin (zaman ve sıcaklık verisi)
                series.Points.AddXY(zaman.ToOADate(), sicaklik);
                chart1.ChartAreas[0].AxisX.Minimum = DateTime.Now.AddSeconds(-10).ToOADate();
                chart1.ChartAreas[0].AxisX.Maximum = DateTime.Now.ToOADate();

                // Çok fazla veri olduğunda eski verileri kaldırarak grafiği temizleyin
                if (series.Points.Count > 100) // Örneğin, 100 veri noktasından fazla olursa
                {
                    series.Points.RemoveAt(0);
                    chart1.ChartAreas[0].RecalculateAxesScale();
                }

                // Grafiği yeniden çiz
                chart1.Invalidate();
            });
        }

        private void UpdateVoltageBar(int index, float voltaj)
        {
            // Invoke kullanarak progress bar güncellemelerini ana iş parçacığına yönlendirin
            this.Invoke((MethodInvoker)delegate
            {
                if (index >= 0 && index < progressBars.Count)
                {
                    // Voltaj değerini progress bar değeri olarak ayarla
                    int progressBarValue = Math.Min(progressBars[index].Maximum, (int)(voltaj * 100));

                    // Debug için kontrol edelim
                    // Debug.WriteLine($"Index: {index}, Voltaj: {voltaj}, ProgressBar Value: {progressBarValue}");

                    progressBars[index].Value = progressBarValue;
                    voltageLabels[index].Text = $"{voltaj:F2} V";
                }
                else
                {
                    MessageBox.Show("Index aralık dışında: " + index);
                }
            });
        }
        private void ReleaseComObject(object obj)
        {
            if (obj != null && Marshal.IsComObject(obj))
            {
                Marshal.ReleaseComObject(obj);
            }
        }

        private void CreateNewLogFile()
        {
            if (writer != null)
            {
                writer.Close();
                writer.Dispose();
            }
            string desktopPath = Environment.GetFolderPath(Environment.SpecialFolder.Desktop);
            string fileName = Path.Combine(desktopPath, $"Veri_{DateTime.Now.ToString("yyyyMMdd_HHmmss")}.txt");
            writer = new StreamWriter(fileName, true);
            writer.WriteLine("zaman_ms;hiz_kmh;T_bat_C;V_bat_C;kalan_enerji_Wh;hucreVoltajlari_V");
            startTime = DateTime.UtcNow;
            millisecondsSinceStart = 0;
            lastDataReceivedTime = DateTime.Now;

        }
        private void SaveDataToFile(string[] veriParcalari, int hiz, float sicaklik)
        {

            if (writer == null)
            {
                CreateNewLogFile();
            }

            millisecondsSinceStart = (long)(DateTime.UtcNow - startTime).TotalMilliseconds;

            StringBuilder sb = new StringBuilder();
            sb.Append($"{millisecondsSinceStart};{hiz};{sicaklik:F2}");

            for (int i = 0; i < 20; i++)
            {
                string voltageStr = veriParcalari[i + 2].Replace('.', ',');
                sb.Append($";{voltageStr}");
            }

            sb.Append($";{veriParcalari[22].Replace('.', ',')};{veriParcalari[23]}");

            writer.WriteLine(sb.ToString());
            writer.Flush(); // Her yazma işleminden sonra buffer'ı temizle
        }

        private void Form1_FormClosed(object sender, FormClosedEventArgs e)
        {
            if (serialPort1.IsOpen)
            {
                serialPort1.Close();
                writer?.Close(); // Dosya yazıcısını kapat
            }

        }

        private void label2_Click(object sender, EventArgs e)
        {

        }
    }
}
