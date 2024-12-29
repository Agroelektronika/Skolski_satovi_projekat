package nemanja.milosevic.zvono;

public class ElementListeRasporeda
{
    private String ime;
    private boolean aktivan = false;

    public ElementListeRasporeda(String imee, boolean akt){
        this.ime = imee;
        this.aktivan = akt;
    }
    public void setIme(String imee){
        this.ime = imee;
    }
    public String getIme(){return this.ime;}
    public boolean getAktivan(){return this.aktivan;}
}
