import requests

def download_unicode_data(url, file_path):
    """Download the UnicodeData.txt file using wget."""
    try:
        # Use requests to download the file
        response = requests.get(url)
        response.raise_for_status()  # Raise an exception for HTTP errors
        with open(file_path, 'w', encoding='utf-8') as f:
            f.write(response.text)
        print(f"Successfully downloaded {file_path}")
    except requests.exceptions.RequestException as e:
        print(f"Error downloading file: {e}")
        exit(1)

def parse_unicode_data(file_path):
    """Parse UnicodeData.txt to extract mappings for lowercase and uppercase."""
    to_lower = []
    to_upper = []
    
    with open(file_path, 'r', encoding='utf-8') as f:
        for line in f:
            fields = line.split(';')
            
            # Check if the line is a valid UnicodeData line (must have at least 15 fields)
            if len(fields) < 15:
                continue
            
            codepoint = int(fields[0], 16)  # Codepoint as an integer
            lowercase_mapping = fields[13]
            uppercase_mapping = fields[12]
            
            # Add to toLower if lowercase_mapping is set
            if lowercase_mapping:
                to_lower.append((codepoint, int(lowercase_mapping, 16)))
            
            # Add to toUpper if uppercase_mapping is set
            if uppercase_mapping:
                to_upper.append((codepoint, int(uppercase_mapping, 16)))
    
    return to_lower, to_upper

def generate_header_file(output_file, to_lower, to_upper):
    """Generate the C header file with toLower and toUpper tables."""
    with open(output_file, 'w', encoding='utf-8') as f:
        # Write the preamble and header
        f.write("#ifndef UNICODE_MAPPINGS_H\n")
        f.write("#define UNICODE_MAPPINGS_H\n\n")
        f.write("#include <stdint.h>\n\n");
        
        # Define the struct for the mappings
        f.write("typedef struct {\n")
        f.write("    uint32_t source;\n")
        f.write("    uint32_t target;\n")
        f.write("} UnicodeMapping;\n\n")
        
        # Define the toLower and toUpper tables as constants
        f.write("static const UnicodeMapping toLower[] = {\n")
        for codepoint, target in sorted(to_lower):
            f.write(f"    {{0x{codepoint:08X}, 0x{target:08X}}},\n")
        f.write("};\n\n")
        
        f.write("static const UnicodeMapping toUpper[] = {\n")
        for codepoint, target in sorted(to_upper):
            f.write(f"    {{0x{codepoint:08X}, 0x{target:08X}}},\n")
        f.write("};\n\n")
        
        # Closing header guard
        f.write("#endif // UNICODE_MAPPINGS_H\n")
    
    print(f"Header file successfully generated: {output_file}")

if __name__ == "__main__":
    # URL for UnicodeData.txt (Unicode version 16.0.0)
    unicode_data_url = "https://www.unicode.org/Public/16.0.0/ucd/UnicodeData.txt"
    
    # Path to save the downloaded UnicodeData.txt
    unicode_data_file = "UnicodeData.txt"
    
    # Output header file name
    output_file = "unicode_mappings.h"
    
    # Download the UnicodeData.txt file
    download_unicode_data(unicode_data_url, unicode_data_file)
    
    # Parse the UnicodeData.txt file to extract mappings
    to_lower, to_upper = parse_unicode_data(unicode_data_file)
    
    # Generate the C header file with the mappings
    generate_header_file(output_file, to_lower, to_upper)