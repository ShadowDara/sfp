use std::pin::Pin;

// C++ Bridge
#[cxx::bridge]
pub mod ffi {

    unsafe extern "C++" {
        include!("mylib.hpp");

        type RustMap;

        fn insert(self: Pin<&mut RustMap>, key: &str, value: String);
        fn rm(self: Pin<&mut RustMap>, key: &str) -> bool;
        fn get(self: &RustMap, key: &str) -> String;
        fn len(self: &RustMap) -> usize;
        fn keys(self: &RustMap) -> Vec<String>;

        pub fn get_sections(value: String) -> UniquePtr<RustMap>;

    }
}

#[derive(Debug, Clone)]
pub struct SectionInfo {
    pub name: String,
    pub start_line: u32,
    pub end_line: u32,
    pub content: String,
}

pub fn parse_v2_sections(text: &str) -> Vec<SectionInfo> {
    const SECTION_KEYWORD: &str = "%%section ";
    const SECTION_END_KEYWORD: &str = "%%endsection";

    let mut sections = Vec::new();

    let mut current: Option<SectionInfo> = None;

    for (line_no, line) in text.lines().enumerate() {
        // %%section NAME
        if let Some(name) = line.strip_prefix(SECTION_KEYWORD) {
            // Falls vorher noch eine Section offen war,
            // schließen wir sie direkt vor dieser neuen Section.
            if let Some(mut section) = current.take() {
                section.end_line = line_no.saturating_sub(1) as u32;
                sections.push(section);
            }

            current = Some(SectionInfo {
                name: name.to_string(),
                start_line: line_no as u32,
                end_line: line_no as u32,
                content: String::new(),
            });

            continue;
        }

        // %%endsection
        if line.starts_with(SECTION_END_KEYWORD) {
            if let Some(mut section) = current.take() {
                section.end_line = line_no as u32;
                sections.push(section);
            }

            continue;
        }

        // Inhalt der aktuellen Section
        if let Some(section) = current.as_mut() {
            section.content.push_str(line);
            section.content.push('\n');
        }
    }

    // Section wurde nicht mit %%endsection geschlossen
    if let Some(mut section) = current {
        section.end_line = text.lines().count().saturating_sub(1) as u32;
        sections.push(section);
    }

    sections
}
