#include "ref_seq.h"

char ref_antd::get_base(int64_t pos, std::string region){
  int len;
  seq = fai_fetch(fai, region.c_str(), &len);
  if(seq == NULL)
    return UNKNOWN_BASE;
  return *(seq + (pos - 1));
}

char* ref_antd::get_codon(int64_t pos, std::string region, gff3_feature feature){
  int len;
  seq = fai_fetch(fai, region.c_str(), &len);
  int64_t edit_pos = feature.get_edit_position(), codon_start_pos;
  std::string edit_sequence = feature.get_edit_sequence();
  char *codon = new char[3];
  int i;
  int64_t edit_offset = 0;
  if(pos > edit_pos + edit_sequence.size() && edit_pos != -1){
    edit_offset = (pos - edit_pos) > edit_sequence.size() ? edit_sequence.size() : (pos - edit_pos); // Account for edits in position of insertion
  }
  codon_start_pos = (feature.get_start() - 1) + feature.get_phase() + (((pos + edit_offset - (feature.get_start() + feature.get_phase())))/3)*3;
  for (i = 0; i < 3; ++i) {
    if(codon_start_pos +i < feature.get_start() - 1 || codon_start_pos +i > feature.get_end() - 1){ // If before or after CDS region return 'N'.
      codon[i] = 'N';
    } else if(codon_start_pos + i < edit_pos - 1 || edit_pos == -1){ // If before edit or with no edit
      codon[i] = *(seq + codon_start_pos + i);
    } else if(codon_start_pos + i >= edit_pos - 1 && codon_start_pos + i <= edit_pos - 1 + edit_sequence.size() - 1){ // size() - 1 since edit_pos include one base already
      codon[i] = edit_sequence[codon_start_pos + i - (edit_pos - 1)];
    } else if(codon_start_pos + i > edit_pos - 1 + edit_sequence.size() - 1) {
      edit_offset = (codon_start_pos + i) - (edit_pos - 1) > edit_sequence.size() ? edit_sequence.size() : (codon_start_pos + i) - (edit_pos - 1);
      codon[i] = *(seq + codon_start_pos + i - edit_offset);
    }
  }
  return codon;
}

char* ref_antd::get_codon(int64_t pos, std::string region, gff3_feature feature, char alt){
  int len;
  seq = fai_fetch(fai, region.c_str(), &len);
  int64_t edit_pos = feature.get_edit_position(), codon_start_pos;
  std::string edit_sequence = feature.get_edit_sequence();
  char *codon = new char[3];
  int i;
  int64_t edit_offset = 0, alt_pos = pos;
  if(pos > edit_pos + edit_sequence.size() && edit_pos != -1){
    edit_offset = (pos - edit_pos) > edit_sequence.size() ? edit_sequence.size() : (pos - edit_pos); // Account for edits in position of insertion
  }
  codon_start_pos = (feature.get_start() - 1) + feature.get_phase() + (((pos + edit_offset - (feature.get_start() + feature.get_phase())))/3)*3;
  for (i = 0; i < 3; ++i) {
    if(codon_start_pos + i < edit_pos - 1 || edit_pos == -1){ // If before edit or with no edit
      codon[i] = *(seq + codon_start_pos + i);
    } else if(codon_start_pos + i >= edit_pos - 1 && codon_start_pos + i <= edit_pos - 1 + edit_sequence.size() - 1){ // size() - 1 since edit_pos include one base already
      codon[i] = edit_sequence[codon_start_pos + i - (edit_pos - 1)];
    } else if(codon_start_pos + i > edit_pos - 1 + edit_sequence.size() - 1) {
      edit_offset = (codon_start_pos + i) - (edit_pos - 1) > edit_sequence.size() ? edit_sequence.size() : (codon_start_pos + i) - (edit_pos - 1);
      codon[i] = *(seq + codon_start_pos + i - edit_offset);
    }
  }
  // Recompute alt position
  edit_offset = 0;
  if(alt_pos > edit_pos + edit_sequence.size() && edit_pos != -1){
    edit_offset = (pos - edit_pos) > edit_sequence.size() ? edit_sequence.size() : (pos - edit_pos); // Account for edits in position of insertion
  }
  alt_pos += edit_offset;
  codon[alt_pos - 1 - codon_start_pos] = alt;
  return codon;
}

int ref_antd::add_gff(std::string path){
  // Read GFF file
  if(!path.empty())
    gff.read_file(path);
  return 0;
}

int ref_antd::add_seq(std::string path){
  // Read reference file
  if(!path.empty())
    fai = fai_load(path.c_str());
  if(!fai && !path.empty()){
    std::cout << "Reference file does not exist at " << path << std::endl;
    return -1;
  }
  return 0;
}

ref_antd::ref_antd(std::string ref_path, std::string gff_path){
  this->seq = nullptr;
  this->add_seq(ref_path);
  this->add_gff(gff_path);
}

int ref_antd::codon_aa_stream(std::string region, std::ostringstream &line_stream, std::ofstream &fout, int64_t pos, char alt){
  std::vector<gff3_feature> features = gff.query_features(pos, "CDS");
  if(features.size() == 0){	// No matching CDS
    fout << line_stream.str() << "\t\t\t\t\t" << std::endl;
    return 0;
  }
  std::vector<gff3_feature>::iterator it;
  char *ref_codon, *alt_codon;
  for(it = features.begin(); it != features.end(); it++){
    fout << line_stream.str() << "\t";
    fout << it->get_attribute("ID") << "\t";
    ref_codon = this->get_codon(pos, region, *it);
    fout << ref_codon << "\t";
    fout << codon2aa(ref_codon[0], ref_codon[1], ref_codon[2]) << "\t";
    alt_codon = this->get_codon(pos, region, *it, alt);
    fout << alt_codon << "\t";
    fout << codon2aa(alt_codon[0], alt_codon[1], alt_codon[2]);
    fout << std::endl;
  }
  return 0;
}

std::vector<gff3_feature> ref_antd::get_gff_features(){
  return gff.get_features();
}
