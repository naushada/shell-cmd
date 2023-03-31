#ifndef __shell_hpp__
#define __shell_hpp__

/**
 * @brief assakeena - the Spirit of Tranquility, or Peace of Reassurance
 * 
 * @date 31st Mar 2023
 */
namespace assakeena {
    enum FDs : std::uint16_t {
        READ = 0,
        WRITE = 1,
        INVALID = 2
    };

    class Shell {
        public:
            Shell() {

            }

            ~Shell() {

            }

            void fds(std::array<std::int32_t, 2> fds) {
                m_fds.push_back(fds);
            }

            auto& fds() {
                return(m_fds);
            }
            auto fds(auto vectorOffset, auto arrayOffset) {
                return(m_fds.at(vectorOffset).at(arrayOffset));
            }

            void fds(auto vectorOffset, auto arrayOffset, auto value) {
                m_fds.at(vectorOffset).at(arrayOffset) = value;
            }

        private:
            std::vector<std::array<std::int32_t, 2>> m_fds;
    };
}















#endif /* __shell_hpp__*/